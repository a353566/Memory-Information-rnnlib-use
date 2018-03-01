#include <dirent.h>
#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>
using namespace std;

string getNumer(char *numChar, int size);

int main() {
  // 資料夾路徑(絕對位址or相對位址) 目前用現在的位置
  string fileName = "transcription@2018.02.13-12.36.09.141337.log";
  
  // 開檔部分
  FILE *file;
  file = fopen(fileName.c_str(),"r");
  if(file == NULL) {
    cout << "open \"" << fileName << "\" File fail" << endl ;
    return false;
  }
  
  char getLine[4096];
  int line=0;
  // 前 278 行跳過
  while (line < 278) {
    if (fgets(getLine, 4096, file) != NULL) {
      line++;
    } else {
      fclose(file);
      return false;
    }
  }
  
  cout << "ctcError | deletions | insertions | labelError | seqError | substitutions |" <<endl;
  string outputStr[12];
  while (fgets(getLine, 4096, file) != NULL) {
    // 讀到 ------------------------------ 才繼續
    while (fgets(getLine, 4096, file) != NULL) {
      line++;
      if (strstr(getLine,"------------------------------") != NULL) {
        break;
      }
    }
    
    // 再跳過 1 行
    if (fgets(getLine, 4096, file) == NULL) { break; }
    // 取數字
    for (int i=0; i<12; i+=2) {
      if (fgets(getLine, 4096, file) != NULL) {
        outputStr[i] = getNumer(getLine, 4096);
      }
    }
    
    // 讀到 ------------------------------ 才繼續
    while (fgets(getLine, 4096, file) != NULL) {
      line++;
      if (strstr(getLine,"------------------------------") != NULL) {
        break;
      }
    }
    
    // 再跳過 1 行
    if (fgets(getLine, 4096, file) == NULL) { break; }
    // 取數字
    for (int i=1; i<12; i+=2) {
      if (fgets(getLine, 4096, file) != NULL) {
        outputStr[i] = getNumer(getLine, 4096);
      }
    }
    
    // 輸出
    for (int i=0; i<12; i++) {
      cout << outputStr[i] << " ";
    }
    cout<<endl;
    
    // 讀到 ------------------------------ 才繼續
    while (fgets(getLine, 4096, file) != NULL) {
      line++;
      if (strstr(getLine,"------------------------------") != NULL) {
        break;
      }
    }
  }
}

string getNumer(char *numChar, int size) {
  bool isGood = false;
  int count = 0;
  string numStr = "";
  for (int i=0; i<size; i++) {
    if (numChar[i] == '\n' || numChar[i] == '\0') {
      break;
    } else if (isGood) {
      if (numChar[i] != ' ') {
        numStr = numStr + numChar[i];
      } else {
        break;
      }
    } else if ('0' <= numChar[i] && numChar[i] <= '9' ) {
      numStr = numChar[i];
      isGood = true;
    }
  }
  return isGood ? numStr:NULL;
}