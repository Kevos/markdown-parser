//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

#include <iostream>
#include <cstdio>

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
    int indent = 2;
    if (argc<3 || (outFile=fopen(argv[1], "w"))==NULL || (markdownFile=fopen(argv[2], "r"))==NULL)
        return 1;
    
    fprintf(outFile, "%s", header);
    
    
    fprintf(outFile, "%s", footer);
    
    
    return 0;
}
