# Wordle Helper
Wordle Helper is a web application used to solve the [Wordle](https://www.nytimes.com/games/wordle/index.html) daily puzzle.

You type a guess into Wordle and put the result in the Helper.
The word you can by typing or using the onscreen keyboard and the colors by either right or left clicking the letters.
Then type enter and the server will calculate the words that fit and out of those, the best guesses.

The algorithm for determining the best guess is taken from this [video by 3Blue1Brown](https://www.youtube.com/watch?v=v68zYyaEmEA).

The application is hosted at [wordlehelper.louisalbanziko.com](wordlehelper.louisalbanziko.com).

This project uses [hermes](https://github.com/LouisAlbanZiko/hermes), my webserver written in Zig, as a backend.
