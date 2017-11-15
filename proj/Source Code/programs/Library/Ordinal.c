char *OrdinalText(int number) {

    static char *text[] = {"th", "st", "nd", "rd"};

    number = (number < 0) ? -number : number;
    if (((number %= 100) > 9 && number < 20) || (number %= 10) > 3) {
        number = 0;
    }
    return text[number];
}
