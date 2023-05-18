# Visual Music Player development roadmap

- kurzor valtozzon at column move eseten
- media player shouldn't draw, it should provide audio info/video bitmap
- multiple file selected/edit/delete
- context menu right click changes songlist scroller
- vertical scroller grab songlist fix
- add last played/last skipped fields
- save shuffle state, current song, current position, current volume on exit, use them on start
- history file - prev button should jump to previously played song
- use mmfm for cover art selection
- green changed, red removed metadata field
- full keyboard control
- genre list should filter artist list also
- filtering with logical operators - genre is metal, year is not 2000
- volume fade in/out on play/pause/next/prev
- metadata update should happen in the backgroun to stop ui lag
- metadata field removal with popup
- drag/resize under floating window managers

TEST

- leak free auto ui tests
- user manual update
- auto test with 2x scaling
- tech video, automated ui/full test, travis test with softrender, logban meg a leakek is lathatoak
- Travis build & test with software rendering

VISUALIZERS

- bar visualiser, fading background
- analog VU meter visualizer with inertia
- modify rdft to show more lower range
- freq visu's left left is weird sometimes, looks inverted
- full screen visualizer
- full screen cover art
- grid-based warping of video/album cover based on frequency ( bass in the center )
- andromeda : monolith 64K demo like particle visualizer  
- cerebral cortex as interactive visualizer - on left/right press start game

STATISTICS

- top 10 most listened artist, song, genre, last month, last year, etc - stats browser
- songs from one year ago this day - history browser
