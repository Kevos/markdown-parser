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

char const *tags[] = {"", "p", "blockquote", "code", "pre", "ul", "ol", "li", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};
char const *templateTags[] = {"html", "head", "body", "style"};
int currentLine = 0, allowChanges = 0, listLevel = 0;
FILE *outFile, *markdownFile;
std::stack<block_enum> blockStack;

void PrintStack(void)
{
    int h[32] = {-1};
    int size =(int)blockStack.size();
    for (int i=size-1; i >=0; --i) {
        h[i] = blockStack.top();
        blockStack.pop();
    }
    printf("Line %02d: ", currentLine);
    if (!blockStack.empty())
        printf("NOT EMPTY, DUN GOOFED");
    for (int i=0; i<size; ++i) {
        printf("%02d, ", h[i]);
        blockStack.push((block_enum)h[i]);
    }
    printf("\n");
}

int main(int argc, const char *argv[])
{
    char line[1024];
    int trimStart = 0;
    char *documentTitle = NULL;
    int embeddedStyles;
    
    if (argc<3) {
        std::cout << "Too few arguments. Usage: " << argv[0] << " fOut fIn [style1 style2 ...]\n";
        return 1;
    }
    if ((markdownFile=fopen(argv[2], "r"))==NULL) {
        std::cout << "Error opening " << argv[2] << " for reading\n";
        return 1;
    }
    if ((outFile=fopen(argv[1], "w"))==NULL) {
        std::cout << "Error opening " << argv[1] << " for writing\n";
        fclose(markdownFile);
        return 1;
    }
    
    embeddedStyles = argc>3&&!strcmp(argv[argc-1], "-embed");
    
    
    
    fprintf(outFile, "<!DOCTYPE html>\n");
    AddToBlockStack(blockHtml, NULL);
    AddToBlockStack(blockHead, NULL);
    
    fgets(line, 1024, markdownFile);
    if (!strncmp(line, "@@ ", 3)) {
        documentTitle = line+3;
    }
    
    Indent();
    fprintf(outFile, "<title>%s</title>\n", documentTitle!=NULL?StripNL(documentTitle):"**Untitled**");
    for (int i=argc-embeddedStyles-1; i>2; --i) {
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
            }
        }
        else {
            Indent();
            fprintf(outFile, "<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\" />\n", argv[i]);
        }
    }
    RemoveFromBlockStack(1);
    AddToBlockStack(blockBody, NULL);
    
    if (documentTitle)
        fgets(line, 1024, markdownFile);
    
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
//    char className[64] = {0};
//    char styles[1024] = {0};
    char tagCustomisation[1024] = "";
    char buffer[1024];
    char *p;
    
    switch (s[0]) {
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
                    changeBlock = 2; // 1
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
            if (allowChanges)
                listLevel = level;
            break;
    }
    
    if (modifier>=0) {
        if (s[modifier]=='^' && (p=strchr(s+modifier+1, '^'))!=NULL) {
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s id=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
        }
        
        if (s[modifier]=='$' && (p=strchr(s+modifier+1, '$'))!=NULL) {
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s class=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
        }
        
        if (s[modifier]=='{' && (p=strchr(s+modifier, '}'))!=NULL) {
            strncpy(buffer, s+modifier+1, p-(s+modifier+1));
            buffer[p-(s+modifier+1)] = '\0';
            sprintf(tagCustomisation, "%s style=\"%s\"", tagCustomisation, buffer);
            modifier = (int)(p-s)+1;
        }
        
        if (blockStack.top()!=blockCode && modifier>=0 && s[modifier]==' ')
            ++modifier;
    }
    
    
    if (allowChanges) {
        if (changeBlock && blockStack.top()<0x30) {
            RemoveFromBlockStack(changeBlock);
        }
        if (newBlock) {
            if (newBlock==blockCode) {
                AddToBlockStack((block_enum)(newBlock+1), NULL);
            }
            AddToBlockStack(newBlock, tagCustomisation);
        }
        allowChanges = 0;
    }
    return modifier;
}

void WriteLine(char *s)
{
    int isBold = 0, isItalic = 0, isCode = 0, isBL = 0, isStrike = 0;
    char linkName[256], linkURL[1024];
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
                    fputc(s[++i], outFile);
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
                case '!':
                    if (i+1<strlen(s) && LinkPresent(s+i+1, linkName, linkURL, &offset) && strlen(linkURL)) {
                        int existsName = (int)strlen(linkName);
                        char formatting[1024];
                        if (existsName)
                            sprintf(formatting, " title=\"%s\" alt=\"%s\"", linkName, linkName);
                        else
                            strcpy(formatting, "");
                        fprintf(outFile, "<img src=\"%s\"%s />", linkURL, formatting);
                        i+=offset+1;
                    }
                    else {
                        fputc(s[i], outFile);
                    }
                    break;
                case '[':
                    if (LinkPresent(s+i, linkName, linkURL, &offset) && strlen(linkURL)) {
                        fprintf(outFile, "<a href=\"%s\">%s</a>", linkURL, linkName);
                        i+=offset;
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
        else
            fputc(s[i], outFile);
    }
    
    for (int i=0; i<isBold+isItalic+isBL+isStrike; ++i) {
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
        *level = offset+1;
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
            *level = offset+1;
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
    fprintf(outFile, "<%s%s>\n", block>=0x30?templateTags[block&0x0F]:tags[block], customisation?customisation:"");
    blockStack.push(block);
}

void RemoveFromBlockStack(int n)
{
    block_enum block;
    
    for (int i=0; i<n; ++i) {
        block = blockStack.top();
        
        blockStack.pop();
        Indent();
        fprintf(outFile, "</%s>\n", block>=0x30?templateTags[block&0x0F]:tags[block]);
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