//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

#include <iostream>
#include <cstdio>

char *StripNL(char *s)
{
    for (int i=(int)strlen(s)-1; i >=0; --i) {
        if (s[i]=='\n') {
            s[i] = '\0';
            break;
        }
    }
    return s;
}

void Indent(FILE *f, int n)
{
    for (int i=0; i<n; i++) {
        fprintf(f, "\t");
    }
}

int main(int argc, const char * argv[])
{
    FILE *outFile, *markdownFile;
    char header[] = "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<title>{TEST}</title>\n\t</head>\n\t<body>\n";
    char footer[] = "\t</body>\n</html>";
    int indentN = 2;
    char line[1024];
    if (argc<3 || (outFile=fopen(argv[1], "w"))==NULL || (markdownFile=fopen(argv[2], "r"))==NULL)
        return 1;
    
    fprintf(outFile, "%s", header);
    while (fgets(line, 1024, markdownFile)) {
        Indent(outFile, indentN);
        fprintf(outFile, "%s", line);
    }
    
    fprintf(outFile, "%s", footer);
    
    
    return 0;
}
