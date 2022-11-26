#!/bin/bash

pandoc -f markdown -t html guide.md -o guide_content.html
cat header.html guide_content.html footer.html > guide.html 

