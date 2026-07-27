import itertools
import re
import struct
import time
from dataclasses import dataclass
import click
import httpx

HEADERS = {
    "User-Agent": "MindMazeArticleChecker/1.0 (congusbongus@gmail.com) httpx/0.27",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
    "Accept-Encoding": "gzip, deflate, br",
    "Connection": "keep-alive",
}


@dataclass
class Answer:
    answer: str
    b1: int
    b2: int
    b3: int
    b4: int
    is_correct: bool
    article: str

    @property
    def wikititle(self) -> str:
        # Transform article name into a nice Wikipedia title
        title = self.article
        # Reorder people names
        if ", " in title:
            spl = title.split(", ")
            # Move the first name to the last place and join with a space
            # Smith, John -> John Smith
            # Norfolk, Thomas Howard, 3rd Duke of -> Thomas Howard, 3rd Duke of Norfolk
            title = ", ".join(spl[1:]) + " " + spl[0]
        # Remove parens (expansion of intialised names)
        return re.sub(r"\(.+\)", "", title).strip()

    @property
    def wikilink(self) -> str:
        # Transform article name into a nice Wikipedia link
        sanitised = self.wikititle.replace(" ", "_")
        return f"https://en.wikipedia.org/wiki/{sanitised}"


@dataclass
class Question:
    question: str
    answers: list[Answer]
    # TODO: category, difficulty

    @property
    def text(self) -> str:
        return self.question.replace("<it>", "<i>").replace("</it>", "</i>")


@click.command()
@click.option(
    "--check_articles",
    is_flag=True,
    help="Check validity of Wikipedia articles.",
)
def main(check_articles: bool):
    questions = load_questions()
    titles = set()
    for index, q in enumerate(questions, 1):
        print(f"\nQuestion {index}: {q.text}")
        for i, a in enumerate(q.answers, 1):
            print(
                f"  {i}: {a.answer} {'✅' if a.is_correct else ''} [{a.wikilink}] [{a.b1} {a.b2} {a.b3} {a.b4}]"
            )
            titles.add(a.wikititle)
    if check_articles:
        valid_articles = set()
        invalid_articles = set()
        try:
            with open(".valid_articles.txt", "r") as f:
                for line in f:
                    valid_articles.add(line.strip())
        except FileNotFoundError:
            pass
        try:
            with open(".invalid_articles.txt", "r") as f:
                for line in f:
                    invalid_articles.add(line.strip())
        except FileNotFoundError:
            pass
        unchecked_articles = titles - valid_articles - invalid_articles
        try:
            with httpx.Client(headers=HEADERS, follow_redirects=True) as client:
                for batch in itertools.batched(unchecked_articles, 50):
                    res = client.get(
                        "https://en.wikipedia.org/w/api.php",
                        params={
                            "action": "query",
                            "format": "json",
                            "titles": "|".join(batch),
                            "redirects": 1,
                            "formatversion": 2,
                        },
                    )
                    res.raise_for_status()
                    if res.is_success:
                        data = res.json()
                        for page in data["query"]["pages"]:
                            title = page["title"]
                            if "missing" not in page:
                                valid_articles.add(title)
                            else:
                                invalid_articles.add(title)
                        time.sleep(0.1)
                    else:
                        print("Wikilink batch", batch)
        except httpx.RequestError:
            pass
        with open(".valid_articles.txt", "w") as f:
            f.write("\n".join(valid_articles))
        with open(".invalid_articles.txt", "w") as f:
            f.write("\n".join(invalid_articles))


def load_questions() -> list[Question]:
    # Load MINDMAZE.DB
    with open("MINDMAZE.DB", "rb") as f:
        data = f.read()

    questions = []
    offset = 0
    while True:
        """
        Load and parse MINDMAZE.DB file
        Data structure:
        uint32_t delimiter; // -1
        uint32_t question_len;
        string question;
        byte answer1Len;
        string answer1;
        uint32_t unknown1;
        byte answer2Len;
        string answer2;
        uint32_t unknown2;
        byte answer3Len;
        string answer3;
        uint32_t unknown3;
        byte answer4Len;
        string answer4;
        uint32_t unknown4;
        """
        # Try to find the delimiter
        while offset < len(data):
            # Read 4 bytes as uint32 (little-endian)
            value = struct.unpack_from("<I", data, offset)[0]
            if value == 0xFFFFFFFF:
                break
            offset += 1
        # Skip the delimiter (4 bytes)
        offset += 4
        question_len = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        question = data[offset : offset + question_len].decode("utf-8", errors="ignore")
        offset += question_len

        print(f"Loaded question: {question}")

        # Read answers
        answers = []
        for i in range(4):
            # Read answer length (1 byte)
            answer_len = struct.unpack_from("<B", data, offset)[0]
            offset += 1

            # Read answer (ISO-8859-2 encoding)
            answer = data[offset : offset + answer_len].decode(
                "latin-1", errors="ignore"
            )

            # Sometimes answers are null terminated - take slice until first null character
            if (null_index := answer.find("\0")) > 0:
                answer = answer[:null_index]

            offset += answer_len

            # Strip and convert XML tags in answers
            answer = answer.replace("<it>", "<i>").replace("</it>", "</i>")

            # Extract text without HTML tags
            article = re.sub(r"<[^>]+>", "", answer).strip()

            # Read 4 unknown bytes
            b1 = struct.unpack_from("<B", data, offset)[0]
            offset += 1
            b2 = struct.unpack_from("<B", data, offset)[0]
            offset += 1
            b3 = struct.unpack_from("<B", data, offset)[0]
            offset += 1
            b4 = struct.unpack_from("<B", data, offset)[0]
            offset += 1

            print(f"- Loaded answer {i + 1}: {answer} {article} {b1} {b2} {b3} {b4}")

            answers.append(Answer(answer, b1, b2, b3, b4, b1 > 0, article))

        questions.append(Question(question, answers))

        if offset >= len(data):
            break

    return questions


if __name__ == "__main__":
    main()
