# genpass

```
Usage: genpass [OPTIONS]
Generates a secure password using random words and symbols

Options:
    -w, --words NUM     Number of words (default: random 5-7)
    -s, --symbols NUM   Number of symbols to add between words
    -n, --numbers NUM   Number of additional numbers to add between words
    --no-suffix        Don't add default trailing numbers
    -h, --help         Show this help message

Example outputs:
    genpass            => Castle_Wizard_Storm_Dragon_Forest757
    genpass -s 2       => Castle#Wizard_Storm@Dragon_Forest333
    genpass -n 2 -s 2  => Castle#Wizard42@Storm_Dragon%Forest989
```