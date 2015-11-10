//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

#include <iostream>
#include <cstdio>
#include <cstring>

#define PARAGRAPH 0x01

#define QBLOCK 0x02
#define CBLOCK 0x03

#define H1 0x0A
#define H2 0x0B
#define H3 0x0C
#define H4 0x0D
#define H5 0x0E
#define H6 0x0F


void Indent(FILE *f, int n);
char *StripNL(char *s);
int ResolveBlock(char *s);
void StartBlock(int block);
void TerminateBlock(void);
void WriteLine(char *s);
void TerminateLine(void);

char const *tags[] = {"", "p", "blockquote", "code", "pre", "", "", "", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};
int const baseIndent = 2;

int currentBlock = 0;
int currentLine = 0;
int indentN = 0;
int allowChanges = 0;
FILE *outFile, *markdownFile;

int main(int argc, const char * argv[])
{
    
    char header1[] = "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<title>{TEST}</title>\n";
    char header2[] = "\t</head>\n\t<body>\n";
    char footer[] = "\t</body>\n</html>";
    char line[1024];
    int trimStart = 0;
    
    
    if (argc<3 || (outFile=fopen(argv[1], "w"))==NULL || (markdownFile=fopen(argv[2], "r"))==NULL)
        return 1;
    
    fprintf(outFile, "%s", header1);
    
    for (int i=argc-1; i>2; --i)
        fprintf(outFile, "\t\t<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\" />\n", argv[i]);
    fprintf(outFile, "%s", header2);
    
    
    while (fgets(line, 1024, markdownFile)) {
        ++currentLine;
        if (currentBlock==CBLOCK && strncmp(line, "```", 3)) {
            allowChanges = 0;
            ResolveBlock(line);
            WriteLine(line);
            fprintf(outFile, "\n");
            continue;
        }
        allowChanges = 1;
        trimStart = ResolveBlock(line);
        if (trimStart>=0) {
            Indent(outFile, indentN);
            WriteLine(line+trimStart);
            TerminateLine();
        }
    }
    allowChanges = 1;
    TerminateBlock();
    fprintf(outFile, "%s", footer);
    
    fclose(outFile);
    fclose(markdownFile);
    return 0;
}

void Indent(FILE *f, int n)
{
    for (int i=0; i<n+baseIndent; i++) {
        fprintf(f, "\t");
    }
}

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

int ResolveBlock(char *s)
{
    int modifier = 0;
    switch (s[0]) {
        case '\n':
            if (!strcmp(s, "\n")) {
                TerminateBlock();
            }
            modifier = -1;
            break;
        case '>':
            modifier = 1;
            if (currentBlock!=QBLOCK) {
                TerminateBlock();
                StartBlock(QBLOCK);
            }
            break;
        case '`':
            if (!strncmp(s, "```", 3)) {
                if (currentBlock==CBLOCK) {
                    TerminateBlock();
                }
                else {
                    TerminateBlock();
                    StartBlock(CBLOCK);
                }
            }
            modifier = -1;
            break;
        case '#':
            modifier = 1;
            for (int i=1; i<6; ++i) {
                if (s[i] != '#')
                    break;
                ++modifier;
            }
            TerminateBlock();
            StartBlock(H1+(modifier-1));
            if (s[modifier]==' ')
                ++modifier;
            break;
        default:
            if (currentBlock>1 && currentBlock!=CBLOCK)
                TerminateBlock();
            if (!currentBlock) {
                StartBlock(PARAGRAPH);
            }
            break;
    }
    if (currentBlock!=CBLOCK && modifier>=0 && s[modifier]==' ')
        ++modifier;
    allowChanges = 0;
    return modifier;
}

void StartBlock(int block)
{
    if (allowChanges) {
        Indent(outFile, indentN++);
        if (block == CBLOCK) {
            fprintf(outFile, "<%s>\n", tags[block+1]);
            Indent(outFile, indentN++);
        }
        fprintf(outFile, "<%s>\n", tags[currentBlock=block]);
    }
}

void TerminateBlock(void)
{
    if (allowChanges && currentBlock) {
        Indent(outFile, --indentN);
        fprintf(outFile, "</%s>\n", tags[currentBlock]);
        if (currentBlock == CBLOCK) {
            Indent(outFile, --indentN);
            fprintf(outFile, "</%s>\n", tags[currentBlock+1]);
        }
        currentBlock = 0;
    }
}

void WriteLine(char *s)
{
    int isBold = 0, isItalic = 0, isCode = 0, isBL = 0;
    
    StripNL(s);
    
    if (currentBlock==CBLOCK) {
        for (int i=0; i<strlen(s); ++i) {
            switch (s[i]) {
                case '<':
                    fprintf(outFile, "&lt;");
                    break;
                case '>':
                    fprintf(outFile, "&gt;");
                    break;
                case '&':
                    fprintf(outFile, "&amp;");
                    break;
                default:
                    fputc(s[i], outFile);
                    break;
            }
        }
//        fprintf(outFile, "%s", s);
        return;
    }
    
    for (int i=0; i<strlen(s); ++i) {
        if (!isCode || s[i]=='`') {
            switch (s[i]) {
                case '\\':
                    fputc(s[++i], outFile);
                    break;
                case '`':
                    fprintf(outFile, "<%scode>", isCode?"/":"");
                    isCode = !isCode;
                    break;
                case '_':
                    if (!strncmp(s+i, "_**", 3) && !isBL) {
                        fprintf(outFile, "<span style=\"font-weight: bold; font-style: italic\">");
                        i += 2;
                        isBL = 1;
                    }
                    break;
                case '*':
                    if (!strncmp(s+i, "**_", 3) && isBL) {
                        fprintf(outFile, "</span>");
                        i += 2;
                        isBL = 0;
                    }
                    else if (i+1<strlen(s) && s[i+1]=='*') {
                        if (isBold)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span style=\"font-weight: bold\">");
                        isBold = !isBold;
                        ++i;
                    }
                    else {
                        if (isItalic)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span style=\"font-style: italic\">");
                        isItalic = !isItalic;
                    }
                    break;
                default:
                    fputc(s[i], outFile);
                    break;
            }
        }
        else
            fputc(s[i], outFile);
    }
    
    for (int i=0; i<isBold+isItalic+isBL; ++i) {
        fprintf(outFile, "</span>");
    }
    
    if (isCode) {
        std::cout << "WARNING (Line " << currentLine << ") -> No closing `\n";
    }
}

void TerminateLine(void)
{
    char nextLine[16];
    int nextStart;
    fpos_t cursor;
    
    fgetpos(markdownFile, &cursor);
    
    fgets(nextLine, 16, markdownFile);
    
    if (feof(markdownFile)) {
        fprintf(outFile, "\n");
    }
    else {
        nextStart = ResolveBlock(nextLine);
        if (currentBlock==PARAGRAPH) {
            if (nextStart==0)
                fprintf(outFile, "<br />\n");
            else if (nextStart!=0)
                fprintf(outFile, "\n");
        }
        else {
            fprintf(outFile, "\n");
        }
    }
    fsetpos(markdownFile, &cursor);
}

