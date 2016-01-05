//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

#include <iostream>
#include <stack>
#include <cstdio>
#include <cstring>

enum block_enum {
    blockNone=0x00, blockP, blockQuote, blockCode, blockPre, blockUl, blockOl, blockLi, blockH1=0x0A, blockH2, blockH3, blockH4, blockH5, blockH6, blockHtml=0x30, blockHead, blockBody, blockStyle
};

void Indent(void);
char *StripNL(char *s);
int IsNumber(char c);
int ResolveBlock(char *s);
void WriteLine(char *s);
void TerminateLine(void);
int IsWhitespace(char c);
int IsListElement(char *s, int *level, int *cutOff);
int IsListBlock(block_enum block);
void AddToBlockStack(block_enum block, char *customisation);
void RemoveFromBlockStack(int n);
void ClearBlocks(void);
int LinkPresent(char *s, char *linkName, char *linkURL, int *offset);
int SpanBlockPresent(char *s, char *styleClass, int *offset);
void RemoveExtension(char *s);

char const *tags[] = {"", "p", "blockquote", "code", "pre", "ul", "ol", "li", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};
char const *templateTags[] = {"html", "head", "body", "style"};
int currentLine = 0, allowChanges = 0, listLevel = 0;
int verbose = 0;
FILE *outFile, *markdownFile;
std::stack<block_enum> blockStack;

int main(int argc, const char *argv[])
{
    char line[1024];
    int trimStart = 0;
    char *documentTitle = NULL;
    int embeddedStyles = 0;
    int noOverwrite = 0;
    int toTerminal = 0;
    int switchOffset = 1;
    char outputName[64] = "", outputFileName[64] = "";
    int outputFileModifier = -1;
    
    while (switchOffset<argc && argv[switchOffset][0]=='-') {
        for (int i=1; i<strlen(argv[switchOffset]); ++i) {
            switch (argv[switchOffset][i]) {
                case 'e':
                    embeddedStyles = 1;
                    break;
                case 'n':
                    noOverwrite = 1;
                    break;
                case 'o':
                    toTerminal = 1;
                    break;
                case 'v':;
                    verbose = 1;
                    break;
                default:
                    std::cerr << "Invalid switch \"" << argv[switchOffset][i] << "\"\n";
                    break;
            }
        }
        ++switchOffset;
    }
    --switchOffset;
    
    if (argc+switchOffset<2) {
        std::cerr << "Too few arguments. Usage: " << argv[0] << " [-enov] fOut fIn [style1 style2 ...]\n";
        return 1;
    }
    if ((markdownFile=fopen(argv[1+switchOffset], "r"))==NULL) {
        std::cerr << "Error opening " << argv[1+switchOffset] << " for reading\n";
        return 1;
    }
    if (toTerminal) {
        outFile = stdout;
    }
    else {
        strcpy(outputName, argv[1+switchOffset]);
        RemoveExtension(outputName);
        do {
            ++outputFileModifier;
            if (outFile) {
                fclose(outFile);
            }
            if (outputFileModifier)
                sprintf(outputFileName, "%s_%d.htm", outputName, outputFileModifier);
            else
                sprintf(outputFileName, "%s.htm", outputName);
        }
        while ((outFile=fopen(outputFileName, "r"))!=NULL && noOverwrite);
        
        if ((outFile=fopen(outputFileName, "w"))==NULL) {
            std::cerr << "Error opening " << outputFileName << " for writing\n";
            fclose(markdownFile);
            return 1;
        }
        if (verbose) {
            std::cout << "Writing to file \"" << outputFileName << "\" (" << (embeddedStyles?"Embedding stylesheets":"Linking to stylesheets") << ")\n";
        }
    }
    
    fprintf(outFile, "<!DOCTYPE html>\n");
    AddToBlockStack(blockHtml, NULL);
    AddToBlockStack(blockHead, NULL);
    
    fgets(line, 1024, markdownFile);
    if (!strncmp(line, "@@ ", 3)) {
        documentTitle = line+3;
    }
    
    Indent();
    fprintf(outFile, "<title>%s</title>\n", documentTitle!=NULL?StripNL(documentTitle):"**Untitled**");
    for (int i=argc-1; i>1+switchOffset; --i) {
        if (embeddedStyles) {
            FILE *fTMP = fopen(argv[i], "r");
            if (fTMP) {
                char buffer[1024];
                AddToBlockStack(blockStyle, NULL);
                while (fgets(buffer, sizeof(buffer), fTMP)) {
                    Indent();
                    fprintf(outFile, "%s", buffer);
                }
                RemoveFromBlockStack(1);
                if (verbose)
                    std::cout << "Embedded \"" << argv[i] << "\"\n";
            }
            else if (verbose)
                std::cout << "File \"" << argv[i] << "\" does not exist\n";
        }
        else {
            Indent();
            fprintf(outFile, "<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\" />\n", argv[i]);
            if (verbose) {
                std::cout << "Linked to stylesheet located at \"" << argv[i] << "\"\n";
            }
        }
    }
    
    if (documentTitle)
        fgets(line, 1024, markdownFile);
    
    if (!strcmp(line, "@$\n")) {
        AddToBlockStack(blockStyle, NULL);
        fgets(line, 1024, markdownFile);
        while (strcmp(line, "$@\n")) {
             Indent();
             fprintf(outFile, "%s", line);
             fgets(line, 1024, markdownFile);
        }
        RemoveFromBlockStack(1);
        fgets(line, 1024, markdownFile);
    }
    
    RemoveFromBlockStack(1);
    AddToBlockStack(blockBody, NULL);
    
    do {
        ++currentLine;
        if (blockStack.top()==blockCode && strncmp(line, "```", 3)) {
            allowChanges = 0;
            ResolveBlock(line);
            WriteLine(line);
            fprintf(outFile, "\n");
            continue;
        }
        allowChanges = 1;
        trimStart = ResolveBlock(line);
        if (trimStart>=0) {
            Indent();
            WriteLine(line+trimStart);
            TerminateLine();
        }
    } while (fgets(line, 1024, markdownFile));
    RemoveFromBlockStack((int)blockStack.size());
    fclose(outFile);
    fclose(markdownFile);
    return 0;
}

void Indent(void)
{
    if (!blockStack.empty()) {
        for (int i=0; i<(int)blockStack.size(); i++) {
            fprintf(outFile, "\t");
        }
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

int IsNumber(char c)
{
    return c>='0'&&c<='9';
}

int ResolveBlock(char *s)
{
    int modifier = 0;
    int changeBlock = 0;
    block_enum newBlock = blockNone;
    char tagCustomisation[1024] = "";
    char buffer[1024];
    char *p;
    
    switch (s[0]) {
        case '\r':
        case '\n':
            if (!strcmp(s, "\n") && allowChanges) {
                ClearBlocks();
            }
            modifier = -1;
            break;
        case '>':
            modifier = 1;
            if (blockStack.top()!=blockQuote) {
                changeBlock = 1;
                newBlock = blockQuote;
            }
            break;
        case '`':
            if (!strncmp(s, "```", 3)) {
                if (blockStack.top()==blockCode) {
                    changeBlock = 1; //2 // 1
                }
                else {
                    changeBlock = 1;
                    newBlock = blockCode;
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
            changeBlock = 1;
            newBlock = (block_enum)(blockH1+(modifier-1));
            break;
        default:
            int level = 0, listType = IsListElement(s, &level, &modifier);
            
            if (allowChanges) {
                if (listType) {
                    if (!listLevel) {
                        ClearBlocks();
                    }
                    if (level>listLevel) {
                        for (int i=listLevel; i<level; ++i) {
                            if (!allowChanges)
                                break;
                            AddToBlockStack(listType==1?blockUl:blockOl, NULL);
                        }
                    }
                    else if (level<listLevel) {
                        changeBlock = (listLevel-level)*2;
                    }
                    else {
                        if (blockStack.top()==blockLi) {
                            if (allowChanges)
                                RemoveFromBlockStack(1);
                        }
                    }
                    newBlock = blockLi;
                }
                else {
                    while (allowChanges && IsListBlock(blockStack.top()))
                        RemoveFromBlockStack(1);
                    if (blockStack.top()>1 && blockStack.top()!=blockCode && blockStack.top()<blockHtml) {
                        changeBlock += 1;
                    }
                    if (blockStack.top()!=1) {
                        newBlock = blockP;
                    }
                }
                listLevel = level;
            }
            break;
    }
    
    if (modifier>=0) {
        if (s[modifier]=='^' && (p=strchr(s+modifier+1, '^'))!=NULL) {
            if (!modifier && !changeBlock) {
                changeBlock = 1;
                newBlock = blockP;
            }
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s id=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
            
        }
        
        if (s[modifier]=='$' && (p=strchr(s+modifier+1, '$'))!=NULL) {
            if (!modifier && !changeBlock) {
                changeBlock = 1;
                newBlock = blockP;
            }
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s class=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
        }
        
        if (s[modifier]=='{' && (p=strchr(s+modifier, '}'))!=NULL) {
            if (!modifier && !changeBlock) {
                changeBlock = 1;
                newBlock = blockP;
            }
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s style=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
        }
        
        if (blockStack.top()!=blockCode && modifier>0 && s[modifier]==' ' && s[modifier+1]!=' ')
            ++modifier;
    }
    
    
    if (allowChanges) {
        if (changeBlock && blockStack.top()<blockHtml) {
            if (newBlock==blockLi)
                RemoveFromBlockStack(changeBlock);
            else
                ClearBlocks();
        }
        if (newBlock) {
            AddToBlockStack(newBlock, tagCustomisation);
        }
        allowChanges = 0;
    }
    return modifier;
}

void WriteLine(char *s)
{
    int isBold = 0, isItalic = 0, isCode = 0, isBL = 0, isStrike = 0, spanCount = 0;
    char data256[256] = "", data1024[1024] = "";
    int offset;
    
    StripNL(s);
    
    if (blockStack.top()==blockCode) {
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
        return;
    }
    
    for (int i=0; i<strlen(s); ++i) {
        if (!isCode || s[i]=='`') {
            switch (s[i]) {
                case '\\':
                    ++i;
                    if (s[i]=='<')
                        fprintf(outFile, "&lt;");
                    else if (s[i]=='>')
                        fprintf(outFile, "&gt;");
                    else if (s[i]=='&')
                        fprintf(outFile, "&amp;");
                    else
                        fputc(s[i], outFile);
                    break;
                case ' ':
                    if (!strncmp(s+i, "    ", 4)) {
                        fprintf(outFile, "&emsp;");
                        i += 3;
                    }
                    else
                        fputc(s[i], outFile);
                    break;
                case '`':
                    fprintf(outFile, "<%scode%s>", isCode?"/":"", isCode?"":" class=\"code-inline\"");
                    isCode = !isCode;
                    break;
                case '_':
                    if (!strncmp(s+i, "_**", 3) && !isBL) {
                        fprintf(outFile, "<span class=\"span-bold span-italic\" style=\"font-weight: bold; font-style: italic\">");
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
                            fprintf(outFile, "<span class=\"span-bold\" style=\"font-weight: bold\">");
                        isBold = !isBold;
                        ++i;
                    }
                    else {
                        if (isItalic)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span class=\"span-italic\" style=\"font-style: italic\">");
                        isItalic = !isItalic;
                    }
                    break;
                case '~':
                    if (i+1<strlen(s) && s[i+1]=='~') {
                        if (isStrike)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span class=\"span-strikethough\" style=\"text-decoration: line-through\">");
                        isStrike = !isStrike;
                        ++i;
                    }
                    break;
                case ']':
                    if (spanCount) {
                        fprintf(outFile, "</span>");
                        --spanCount;
                    }
                    else
                        fputc(s[i], outFile);
                    break;
                case '$':
                    if (SpanBlockPresent(s+i, data1024, &offset) && strlen(data1024)) {
                        if (*data1024=='^')
                            fprintf(outFile, "<span style=\"%s\">", data1024+1);
                        else
                            fprintf(outFile, "<span class=\"%s\">", data1024);
                        i+=offset;
                        ++spanCount;
                    }
                    else {
                        fputc(s[i], outFile);
                    }
                    break;
                case '!':
                    if (i+1<strlen(s) && LinkPresent(s+i+1, data256, data1024, &offset) && strlen(data1024)) {
                        int existsName = (int)strlen(data256);
                        char formatting[1024];
                        if (existsName)
                            sprintf(formatting, " title=\"%s\" alt=\"%s\"", data256, data256);
                        else
                            strcpy(formatting, "");
                        fprintf(outFile, "<img src=\"%s\"%s />", data1024, formatting);
                        i+=offset+1;
                    }
                    else {
                        fputc(s[i], outFile);
                    }
                    break;
                case '[':
                    if (LinkPresent(s+i, data256, data1024, &offset) && strlen(data1024)) {
                        fprintf(outFile, "<a href=\"%s\">%s</a>", data1024, data256);
                        i+=offset;
                        ++spanCount;
                    }
                    else {
                        fputc(s[i], outFile);
                    }
                    break;
                default:
                    fputc(s[i], outFile);
                    break;
            }
        }
        else {
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
    }
    
    for (int i=0; i<isBold+isItalic+isBL+isStrike+spanCount; ++i) {
        fprintf(outFile, "</span>");
    }
    
    if (isCode && verbose) {
        std::cerr << "WARNING (Line " << currentLine << ") -> No closing `\n";
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
        if (blockStack.top()==blockP) {
            if (nextStart==0)
                fprintf(outFile, "<br />\n");
            else
                fprintf(outFile, "\n");
        }
        else if (blockStack.top()==blockQuote) {
            if (nextLine[0]=='>')
                fprintf(outFile, "<br />\n");
            else
                fprintf(outFile, "\n");
        }
        else {
            fprintf(outFile, "\n");
        }
    }
    fsetpos(markdownFile, &cursor);
}

int IsWhitespace(char c)
{
    char whiteChars[] = {0x20, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0};
    for (int i=0; i<strlen(whiteChars); ++i) {
        if (c==whiteChars[i])
            return 1;
    }
    return 0;
}

int IsListElement(char *s, int *level, int *cuttOff)
{
    int i=0;
    int offset = 0;
    while (IsWhitespace(s[offset]))
        ++offset;
    if (s[offset]=='-') {
        *level = offset/4+1;
        *cuttOff = offset+1;
        return 1;
    }
    else {
        i = offset;
        while (IsNumber(s[i]))
            ++i;
        if (!i || s[i]!='.') {
            *level = 0;
            return 0;
        }
        else {
            *level = offset/4+1;
            *cuttOff = i+1;
            return 2;
        }
    }
}

int IsListBlock(block_enum block)
{
    return block==blockUl||block==blockOl||block==blockLi;
}

void AddToBlockStack(block_enum block, char *customisation)
{
    Indent();
    fprintf(outFile, "%s<%s%s>\n", block==blockCode?"<pre>":"", block>=0x30?templateTags[block&0x0F]:tags[block], customisation?customisation:"");
    blockStack.push(block);
}

void RemoveFromBlockStack(int n)
{
    block_enum block;
    
    for (int i=0; i<n; ++i) {
        block = blockStack.top();
        
        blockStack.pop();
        Indent();
        fprintf(outFile, "</%s>%s\n", block>=0x30?templateTags[block&0x0F]:tags[block], block==blockCode?"</pre>":"");
    }
}

void ClearBlocks(void)
{
    while (blockStack.top()<blockHtml)
        RemoveFromBlockStack(1);
    listLevel = 0;
}

int LinkPresent(char *s, char *linkName, char *linkURL, int *offset)
{
    char *p, *q;
    if (s[1]=='-' && (p=strstr(s+2, "-](")) && strlen(p)>=3 && (q=strchr(p+3, ')'))) {
        strncpy(linkName, s+2, p-(s+2));
        strncpy(linkURL, p+3, q-(p+3));
        *offset = (int)(q-s);
        return 1;
    }
    return 0;
}

int SpanBlockPresent(char *s, char *styleClass, int *offset)
{
    char *p = strchr(s+1, '$');
    if (p && p[1]=='[') {
        int openB = 1, closeB = 0;
        for (int i=2; i<strlen(p); ++i) {
            if (p[i]=='[' && p[i-1]!='\\')
                ++openB;
            else if (p[i]==']' && p[i-1]!='\\')
                ++closeB;
        }
        if (closeB>=openB) {
            strncpy(styleClass, s+1, p-(s+1));
            *offset = (int)(p-s)+1;
            return 1;
        }
    }
    return 0;
}

void RemoveExtension(char *s)
{
    for (int i=(int)strlen(s)-1; i>=0; --i) {
        if (s[i]=='.') {
            s[i] = '\0';
            break;
        }
    }
}