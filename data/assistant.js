function strStrip(str)
{
    return str.replace(/^\s*(.*?)\s*$/, "$1");
}

function strCompactWhitespace(str)
{
    var length;
    var i;
    var ret;
    var whitespace_count = 0;

    str = str.replace(/\t/, " ");

    ret = "";
    length = str.length;
    i = 0;
    while (i < length) {
        if (str[i] == " ") {
            whitespace_count++;
        } else {
            if (whitespace_count > 0) {
                ret += " ";
                whitespace_count = 0;
            }
            ret += str[i];
        }

        i++;
    }

    return ret;
}

function getIndexOfParen(str)
{
    var length = str.length;

    for (var i = 0; i < length; i++) {
        if (str[i] == "(")
            return i;
    }

    return 0;
}

function getIndexOfLastWord(str)
{
    for (var i = str.length; i >= 0; i--) {
        if (str[i] == " ")
            return i + 1;
    }

    return -1;
}

function getIndexOfLastWordIgnoreAsterisk(str)
{
    for (var i = str.length; i >= 0; i--) {
        if (str[i] == " " || str[i] == "*")
            return i + 1;
    }

    return -1;
}

function buildPadding(length)
{
    var str = "";

    while (length > 0) {
        str += " ";
        length--;
    }

    return str;
}

function reformatSignature()
{
    var elements = document.getElementsByClassName('programlisting');

    listing = elements[0];

    /* Fixup oddly formatted HTML, e.g libxml has <br> inside the pre
     * element.
     */
    tmp = listing.innerHTML;
    tmp = tmp.replace("<br>", "\n").replace("\t", " ");
    listing.innerHTML = tmp;

    var input = listing.textContent;
    var lines = input.split("\n");
    var line;
    var i;

    i = 0;
    while (line = lines[i]) {
        lines[i] = strCompactWhitespace(strStrip(line));
        i++;
    }

    var indexOfParen = getIndexOfParen(lines[1]);
    var lastWordIndices = Array(lines.length);
    var maxIndexOfLastWord = 0;
    var maxDiff = 0;

    i = 1;
    while (line = lines[i]) {
        lastWordIndices[i] = getIndexOfLastWordIgnoreAsterisk(line);
        tmp = getIndexOfLastWord(line);

        if (i > 1) {
            lastWordIndices[i] += indexOfParen + 1;
            tmp += indexOfParen + 1;
        }

        if (tmp > maxIndexOfLastWord)
            maxIndexOfLastWord = tmp;

        if (lastWordIndices[i] - tmp > maxDiff)
            maxDiff = lastWordIndices[i] - tmp;

        i++;
    }

    maxIndexOfLastWord += maxDiff;

    // Now get the formatted text.
    var formattedLines = listing.innerHTML.split("\n");

    i = 0;
    while (line = formattedLines[i]) {
        formattedLines[i] = strCompactWhitespace(strStrip(line));
        i++;
    }

    var formattedLastWordIndices = Array(formattedLines.length);

    i = 1;
    while (line = formattedLines[i]) {
        formattedLastWordIndices[i] = getIndexOfLastWord(line);

        if (i > 1)
            formattedLastWordIndices[i] += indexOfParen + 1;

        i++;
    }

    padding = buildPadding(indexOfParen + 1);
    i = 2;
    while (line = formattedLines[i]) {
        formattedLines[i] = padding + line;
        i++;
    }

    i = 1;
    while (line = formattedLines[i]) {
        padding = buildPadding(maxIndexOfLastWord - lastWordIndices[i]);
        formattedLines[i] = line.substr(0, formattedLastWordIndices[i]) +
            padding + line.substr(formattedLastWordIndices[i]);

        i++;
    }

    var output = "";
    i = 0;
    while (line = formattedLines[i]) {
        output = output + line + "\n";
        i++;
    }

    listing.innerHTML = output;
}

function cleanupSignature()
{
    var elements = document.getElementsByClassName('programlisting');

    listing = elements[0];

    var input = listing.innerHTML;
    var lines = input.split("\n");

    var line;
    var i = 0;
    while (line = lines[i]) {
        lines[i] = strCompactWhitespace(strStrip(line));
        i++;
    }

    var output = "";
    i = 0;
    while (line = lines[i]) {
        output = output + line + "\n";
        i++;
    }

    listing.innerHTML = output;
}
