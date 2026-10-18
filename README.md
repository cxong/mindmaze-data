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
 
### Listing of files
 
Note: some files are missing between the MMBAG and ITS/EIT files, they may be unused

| File | Use |
| --- | --- |
| alchem.dib | Character, right of screen (hides north and right door) |
| asiantra.dib | Character, right side |
| atrium.dib | End game screen |
| banner.dib | Modal overlay for quiz question, displayed on the left of the screen |
| BGLOOP1-3.MID | Plays during the main maze game |
| bookshlf.dib | Prop, left of north door |
| broom.dib | Prop, left wall over left door |
| candle1.dib | Prop, right wall near corner and ceiling |
| candle2.dib | Prop, centre left |
| carpet1-2.dib / flrcovr1.dib | Different carpets, sometimes drawn |
| ceiling1-3.dib | Different options for ceilings |
| chkfloor.dib | Alt floor |
| dialog.dib | Modal overlay for speaking to characters, displayed on left half of the screen |
| doorlt.dib / doorrt.dib / ndoor.dib / sdoor.dib | Doors, clicking moves in that direction |
| doortow.dib | End level, drawn over dunrm.dib |
| dtable1-2.dib | Prop, left/right wall |
| duke1.dib | Character, left of screen |
| duke2.dib | Character, right side of screen |
| duke3.dib | Character, left corner |
| end1-6.dib | Message shown at end (unknown what controls which to show) |
| furnitur.dib | Prop, slight left of centre of room |
| grating.dib | Shown after starting game, above UI. Clicking once replaces it with rmofdoor.dib |
| INTRO.MID | Splash screen |
| jester1.dib | Character, right side of room |
| jester2.dib | Character, right of screen |
| jester3.dib | Character, left side of room |
| jester4.dib | Character, right side of room |
| king1.dib | Character, right of room |
| lady1.dib | Character, left of room, hides left door |
| logonscr.dib | Shown after splash screen; names shown on bottom right corner (can create new players or delete), clicking question mark opens modal showing open1/2.dib. Clicking on door starts game |
| maninst.dib | Character, centre of room, blocks north and south |
| metravel.dib | Character, between left prop and north door, in middle of floor |
| parrot.dib | Character, centre left of screen |
| pic1-X.dib | Prop, left side of north wall |
| pic2-X.dib | Prop, right side of back wall, corresponds to areas of interest, clicking opens an article |
| plnwalls.dib / dunrm.dib | Walls, always drawn |
| queen.dib | Character, middle of room |
| QUESTION.MID | Plays when quiz is shown |
| right.wav / wrong.wav | Plays when answer is right/wrong |
| rmofdoor.dib | Shown after starting game and dismissing grating.dib. Different doors are clickable but has no effect (doesn't change maze) |
| secnldy1.dib | Character, left of room |
| secnldy2.dib | Character, right of room  |
| servant1.dib | Character, right of room just below side doors |
| servant2.dib | Character, bottom right of room |
| shelf.dib | Prop, to the top left of left door |
| sorceres.dib | Character, middle of room |
| suitarm1.dib | Character, right of room |
| suitarm2.dib | Character, right side of back wall near corner |
| table1.dib | Prop, left side of screen |
| table2.dib | Prop, below right door |
| titlescr.dib | Splash screen |
| trunk.dib | Prop, left wall below left door |
| vase1.dib | Prop, left corner |
| vase2.dib | Prop, bottom right |
| vase4.dib | Prop, left corner |
| walltre1.dib | Prop, left wall near top |
| walltre2.dib | Prop, right wall near top |

## mmbag_extract

C program that can read MMBAG files, used in older mindmaze versions

## question_viewer

Python script that decodes quiz questions.

```commandline
uv run question_viewer.py
```