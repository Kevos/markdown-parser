//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

#include <iostream>
#include <cstdio>

#define BASEINDENT 2

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
void StartBlock();
void TerminateBlock();


int currentBlock = 0;
int indentN = 0;
FILE *outFile, *markdownFile;
char *tags[] = {"", "p", "blockquote", "pre", "code", "", "", "", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};

int main(int argc, const char * argv[])
{
    
    char header[] = "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<title>{TEST}</title>\n\t</head>\n\t<body>\n";
    char footer[] = "\t</body>\n</html>";
    fpos_t cursor;
    char line[1024];
    int trimStart = 0;
    
    
    if (argc<3 || (outFile=fopen(argv[1], "w"))==NULL || (markdownFile=fopen(argv[2], "r"))==NULL)
        return 1;
    
    fprintf(outFile, "%s", header);
    while (fgets(line, 1024, markdownFile)) {
        trimStart = ResolveBlock(line);
        if (trimStart>=0) {
            Indent(outFile, indentN);
            fprintf(outFile, "%s", StripNL(line+trimStart));
            
            fgetpos(markdownFile, &cursor);
            
            if (fgetc(markdownFile)=='\n' || feof(markdownFile)) {
                fprintf(outFile, "\n");
            }
            else {
                fprintf(outFile, "<br />\n");
            }
            
            fsetpos(markdownFile, &cursor);
        }
        
        
    }
    TerminateBlock();
    fprintf(outFile, "%s", footer);
    
    
    return 0;
}

void Indent(FILE *f, int n)
{
    for (int i=0; i<n+BASEINDENT; i++) {
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
        case '>':
            if (s[1]==' ' && currentBlock!=QBLOCK) {
                TerminateBlock();
                currentBlock = QBLOCK;
                StartBlock();
                modifier = 2;
            }
            break;
        case '`':
            if (!strncmp(s, "```", 3)) {
                if (currentBlock==CBLOCK) {
                    TerminateBlock();
                }
                else {
                    currentBlock = CBLOCK;
                    StartBlock();
                }
            }
            modifier = 3;
            break;
        default:
            if (currentBlock>1)
                TerminateBlock();
            if (!currentBlock) {
                currentBlock = PARAGRAPH;
                StartBlock();
            }
            
            break;
    }
    
    return modifier;
}

void StartBlock()
{
    Indent(outFile, indentN);
    fprintf(outFile, "<%s>\n", tags[currentBlock]);
    ++indentN;
}

void TerminateBlock()
{
    if (currentBlock) {
        --indentN;
        Indent(outFile, indentN);
        fprintf(outFile, "</%s>\n", tags[currentBlock]);
        currentBlock = 0;
    }
}