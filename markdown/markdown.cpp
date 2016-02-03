/*
 markdown (name subject to change) is a simple Markdown to HTML/CSS parser. The Markdown language that is parsed is based of the general Markdown language/conventions but some aspects have changed/altered.
 
 Author: Kevin Hira, http://github.com/Kevos
 */

#include <iostream>
#include <stack>
#include <cstdio>
#include <cstring>

//Enumeration to siginify different HTML blocks (used in conjuntion with a stack)
enum block_enum {
    blockNone=0x00, blockP, blockQuote, blockCode, blockPre, blockUl, blockOl, blockLi, blockH1=0x0A, blockH2, blockH3, blockH4, blockH5, blockH6, blockHtml=0x30, blockHead, blockBody, blockStyle
};

void Indent(void);
int StripNL(char *s);
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
int IsHTMLCode(char *s, int *offset);
void EscapeCharacter(char c);

char const *tags[] = {"", "p", "blockquote", "code", "pre", "ul", "ol", "li", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};
char const *templateTags[] = {"html", "head", "body", "style"};
int allowChanges = 0, listLevel = 0, indentOffset = 0;
int verbose = 0;
int openTag = 0, closeTag = 0, plainWrite = 0;
FILE *outFile, *markdownFile;

//Create a HTML block hierarchy stack.
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
    int outputFileModifier = 0;
    
    //Search command for valid program arguments and handle them.
    for (switchOffset=1; switchOffset<argc && argv[switchOffset][0]=='-'; ++switchOffset) {
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
                    if (verbose)
                        std::cerr << "Invalid switch \"" << argv[switchOffset][i] << "\"\n";
                    break;
            }
        }
    }
    
    //Check if there is at least a source file in the command, otherwise the command is not valid.
    if (argc-switchOffset<1) {
        std::cerr << "Too few arguments. Usage: " << argv[0] << " [-enov] fOut fIn [style1 style2 ...]\n";
        return 1;
    }
    
    //Determine whether the source file exists.
    if ((markdownFile=fopen(argv[switchOffset], "r"))==NULL) {
        std::cerr << "Error opening " << argv[switchOffset] << " for reading\n";
        return 1;
    }
    
    //Determine whether the output is written to file or to the console (stdout).
    if (toTerminal) {
        //Point the output file to stdout.
        outFile = stdout;
    }
    else {
        //Copy the source filename from the command and remove the extenstion.
        strcpy(outputName, argv[switchOffset]);
        RemoveExtension(outputName);
        
        //Append .htm to the end.
        sprintf(outputFileName, "%s.htm", outputName);
        
        // If no overwriting has been set, check if the file file exists by tryin to open it.
        while (noOverwrite && (outFile=fopen(outputFileName, "r"))!=NULL) {
            //Increment offset variable, close file.
            ++outputFileModifier;
            fclose(outFile);
            
            //Add new offset to filename.
            sprintf(outputFileName, "%s_%d.htm", outputName, outputFileModifier);
        }
        
        //Do a final check to see if the file can be written to.
        if ((outFile=fopen(outputFileName, "w"))==NULL) {
            std::cerr << "Error opening " << outputFileName << " for writing\n";
            fclose(markdownFile);
            return 1;
        }
        
        if (verbose) {
            std::cout << "Writing to file \"" << outputFileName << "\" (" << (embeddedStyles?"Embedding stylesheets":"Linking to stylesheets") << ")\n";
        }
    }
    
    //Write out header of HTML file.
    fprintf(outFile, "<!DOCTYPE html>\n");
    
    //Add HTML and head blocks to the stack.
    AddToBlockStack(blockHtml, NULL);
    AddToBlockStack(blockHead, NULL);
    
    //Read in first line of source file.
    fgets(line, 1024, markdownFile);
    
    //If this line starts with "@@" treat it as defining the title of the HTML page.
    if (!strncmp(line, "@@ ", 3)) {
        documentTitle = line+3;
        StripNL(documentTitle);
    }
    
    //Write out the title of the page.
    Indent();
    fprintf(outFile, "<title>%s</title>\n", documentTitle!=NULL?documentTitle:"Untitled");
    
    //The reset of the arguments are stylesheet references, loop though theses.
    for (int i=argc-1; i>switchOffset; --i) {
        //If embedded stylesheet are wanted, copy the file contents to the HTML page.
        if (embeddedStyles) {
            FILE *fTMP = fopen(argv[i], "r");
            if (fTMP) {
                char buffer[1024];
                
                //Add the style block to the stack
                AddToBlockStack(blockStyle, NULL);
                
                //Read content and write it out (indented) to the HTML page.
                while (fgets(buffer, sizeof(buffer), fTMP)) {
                    Indent();
                    fprintf(outFile, "%s", buffer);
                }
                
                //Remove style block from stack.
                RemoveFromBlockStack(1);
                
                if (verbose)
                    std::cout << "Embedded \"" << argv[i] << "\"\n";
            }
            else if (verbose)
                std::cout << "File \"" << argv[i] << "\" does not exist\n";
        }
        else {
            //Simply create a link reference to the stylesheet.
            Indent();
            fprintf(outFile, "<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\" />\n", argv[i]);
            
            if (verbose) {
                std::cout << "Linked to stylesheet located at \"" << argv[i] << "\"\n";
            }
        }
    }
    
    
    //If the first line of the souce file defined the title of the HTML page, get the next line.
    if (documentTitle)
        fgets(line, 1024, markdownFile);
    
    
    //If this line starts with "@$" treat it as defining the the start of the custom head HTML code.
    if (!strcmp(line, "@$\n")) {
        //Print contents of between "@$" and "$@" lines of the source file under this style block.
        fgets(line, 1024, markdownFile);
        while (strcmp(line, "$@\n")) {
             Indent();
             fprintf(outFile, "%s", line);
             fgets(line, 1024, markdownFile);
        }
        
        //Get the next line from source file.
        fgets(line, 1024, markdownFile);
    }
    
    //Remove head block from stack and add body block to stack.
    RemoveFromBlockStack(1);
    AddToBlockStack(blockBody, NULL);
    
    //Start writing out the body/rest of the HTML page.
    do {
        
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
    
    //Remove any remaining blocks from stack.
    RemoveFromBlockStack((int)blockStack.size());
    
    //Close the files used.
    fclose(outFile);
    fclose(markdownFile);
    
    return 0;
}
/*
 Indent() writed out indentation (using tab characters) to the file "outFile" dependent on how mant block are in the stack and takes in accound indentation for raw HTML that has been added.
 */
void Indent(void)
{
    for (int i=0; i<(int)blockStack.size()+indentOffset-openTag; ++i)
        fprintf(outFile, "    ");
}

/*
 StripNL() scans through a string from back to front, and replaces the first instance of a newline character (\n) with the null character (\0), which effectively removes the new line suffix to the string. This function takes into account LF and CRLF endline styles, but not CR.
 */
int StripNL(char *s)
{
    //Scan through string from back to front
    for (int i=(int)strlen(s)-1; i >=0; --i) {
        if (s[i]=='\n') {
            s[i] = '\0';
            if (i>0 && s[i-1]=='\r')
                s[i-1] = '\0';
            return 1;
        }
    }
    return 0;
}

/*
 IsNumber() simply returns if a character has integer equivalence between that of a 0 character and a 9 character inclusive.
 */
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
    
    openTag = closeTag = 0, plainWrite = 0;
    
    
    switch (IsHTMLCode(s, &modifier)) {
        case 1:
            if (allowChanges) {
                ClearBlocks();
                ++indentOffset;
                allowChanges = 0;
            }
            openTag = 1;
            return modifier;
        case 2:
            if (allowChanges) {
                ClearBlocks();
                if (--indentOffset<0)
                    indentOffset = 0;
                allowChanges = 0;
            }
            closeTag = 1;
            return modifier;
        case 3:
            if (allowChanges) {
                ClearBlocks();
                allowChanges = 0;
            }
            plainWrite = 1;
            return modifier;
        default:
            if (indentOffset) {
                plainWrite = 1;
                return modifier;
            }
            break;
    }
    
    modifier = 0;
    
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
                        changeBlock = (listLevel-level)*2+1;
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
    int newLineExists;
    int isBold = 0, isItalic = 0, isCode = 0, isBL = 0, isStrike = 0, spanCount = 0;
    char data256[256] = "", data1024[1024] = "";
    char line[1024];
    int offset;
    
    strcpy(line, s);
    
    do {
        
        newLineExists = StripNL(line);
        
        if (blockStack.top()==blockCode) {
            for (int i=0; i<strlen(line); ++i)
                EscapeCharacter(line[i]);
            continue;
        }
        if (openTag || closeTag || plainWrite) {
            fprintf(outFile, "%s", line);
            continue;
        }
        
        for (int i=0; i<strlen(line); ++i) {
            if (!isCode || line[i]=='`') {
                switch (line[i]) {
                    case '\\':
                            if (i+1<strlen(line) && strchr("<>&", line[i+1]))
                                EscapeCharacter(line[++i]);
                            else
                                fputc(line[i], outFile);
                        break;
                    case ' ':
                        if (!strncmp(line+i, "    ", 4)) {
                            fprintf(outFile, "&emsp;");
                            i += 3;
                        }
                        else
                            fputc(line[i], outFile);
                        break;
                    case '`':
                        fprintf(outFile, "<%scode%s>", isCode?"/":"", isCode?"":" class=\"code-inline\"");
                        isCode = !isCode;
                        break;
                    case '_':
                        if (!strncmp(line+i, "_**", 3) && !isBL) {
                            fprintf(outFile, "<span class=\"span-bold span-italic\" style=\"font-weight: bold; font-style: italic\">");
                            i += 2;
                            isBL = 1;
                        }
                        break;
                    case '*':
                        if (!strncmp(line+i, "**_", 3) && isBL) {
                            fprintf(outFile, "</span>");
                            i += 2;
                            isBL = 0;
                        }
                        else if (i+1<strlen(line) && line[i+1]=='*') {
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
                        if (i+1<strlen(line) && line[i+1]=='~') {
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
                            fputc(line[i], outFile);
                        break;
                    case '$':
                        if (SpanBlockPresent(line+i, data1024, &offset)) {
                            if (!strlen(data1024))
                                fprintf(outFile, "<span>");
                            else if (*data1024=='^')
                                fprintf(outFile, "<span style=\"%s\">", data1024+1);
                            else
                                fprintf(outFile, "<span class=\"%s\">", data1024);
                            i += offset;
                            ++spanCount;
                        }
                        else {
                            fputc(line[i], outFile);
                        }
                        break;
                    case '!':
                        if (i+1<strlen(line) && LinkPresent(line+i+1, data256, data1024, &offset) && strlen(data1024)) {
                            int existsName = (int)strlen(data256);
                            char formatting[1024];
                            if (existsName)
                                sprintf(formatting, " title=\"%s\" alt=\"%s\"", data256, data256);
                            else
                                strcpy(formatting, "");
                            fprintf(outFile, "<img src=\"%s\"%s />", data1024, formatting);
                            i += offset+1;
                        }
                        else {
                            fputc(line[i], outFile);
                        }
                        break;
                    case '[':
                        if (LinkPresent(line+i, data256, data1024, &offset) && strlen(data1024)) {
                            fprintf(outFile, "<a href=\"%s\">%s</a>", data1024, data256);
                            i += offset+1;
                        }
                        else {
                            fputc(line[i], outFile);
                        }
                        break;
                    default:
                        fputc(line[i], outFile);
                        break;
                }
            }
            else {
                EscapeCharacter(line[i]);
            }
        }
    } while (!newLineExists && fgets(line, 1024, markdownFile));
    
    if (isCode)
        fprintf(outFile, "</code>");
    
    for (int i=0; i<isBold+isItalic+isBL+isStrike+spanCount; ++i)
        fprintf(outFile, "</span>");
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
        if (openTag || closeTag || plainWrite) {
            fprintf(outFile, "\n");
        }
        else if (blockStack.top()==blockP) {
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

/*
 IsWhitespace() checks to see if a a character is a special whitespace character.
 */
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

int IsHTMLCode(char *s, int *offset)
{
    int firstNonSpace = 0;
    
    for (; firstNonSpace<strlen(s); ++firstNonSpace)
        if (s[firstNonSpace]!=' ' && s[firstNonSpace]!='\t')
            break;
    if (s[firstNonSpace]=='@') {
        *offset = firstNonSpace+1;
        return 3;
    }
    if (s[firstNonSpace]=='<'/* && s[strlen(s)-2]=='>'*/) {
        *offset = firstNonSpace;
        if (s[firstNonSpace+1]=='/')
            return 2;
        return 1;
    }
    return 0;
}

void EscapeCharacter(char c)
{
    switch (c) {
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
            fputc(c, outFile);
            break;
    }
}