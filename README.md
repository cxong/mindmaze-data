# mindmaze-data

Data and reverse engineering for Encarta Mindmaze

## 95-07

Mindmaze-related files from Encarta CDs over the years. The first Encarta version contained a different version of Mindmaze.

 - MINDMAZE.DB - contains questions, see question_viewer
 - MMBAG - from 95-00, mediaview? files, contains data files, see mmbag_extract
 - MINDMAZE.ITS / MINDMAZE.EIT - contains data files, open with 7zip

TODO: collect all versions of Encarta

## baggage

Extracted data files from Mindmaze; these are afaict identical across the versions

 - dib - images
 - wav - sounds
 - mid - music

TODO: listing of files, where they are used

## mmbag_extract

C program that can read MMBAG files, used in older mindmaze versions

## question_viewer

Python script that decodes quiz questions.

```commandline
uv run question_viewer.py
```