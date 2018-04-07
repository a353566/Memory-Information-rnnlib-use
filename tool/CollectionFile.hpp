#ifndef COLLECTION_FILE_HPP
#define COLLECTION_FILE_HPP

#include <dirent.h>
#include <stdio.h>
#include <list>
#include <vector>
#include <string.h>
#include <iostream>

#include "DateTime.hpp"
#include "StringToNumber.hpp"

//#define NEW

using namespace std;

/** class Point
 * 一個時間點的狀況
 * 主要是給 CollectionFile 來紀錄一連串時間用的
 * 但是目前 read.cpp::CollectionAllData 也有拿來用
 * 其中會有 app[] 來記錄當下時間點的所有 app 的狀況
 */
class Point {
  public :
    /** class App
     * 單一 app 的所有資訊 
     * 其中名字是用 point 的方式指向 appNameVec 其中一個真正的名字
     * name|pid|TotalPss|oom_score|ground|oom_adj
     * TotalPss|oom_score|ground|oom_adj
     */
    class App {
      public :
        int namePoint;
        int pid;
        int totalPss;
        int oom_score;
        int ground;   // ground 是 -1 的話代表沒有資料
        int oom_adj;  // oom_adj 是 -10000 的話代表沒有資料
        
        App() {
          namePoint = -1;
          pid = -1;
          totalPss = -1;
          oom_score = -10000;
          ground = -1;
          oom_adj = -10000;
        }
        
        void output() {
          cout << "namePoint:" << namePoint
            << "\tpid:" << pid
            << "\ttotalPss:" << totalPss
            << "\toom_score:" << oom_score
            << "\tground:" << ground
            << "\toom_adj:" << oom_adj <<endl;
        };
        void output(const vector<string> *appNameVec) {
          cout << "name:" << (*appNameVec)[namePoint] << '\n'
            << "pid:" << pid
            << "\ttotalPss:" << totalPss
            << "\toom_score:" << oom_score
            << "\tground:" << ground
            << "\toom_adj:" << oom_adj <<endl;
        };
    };
    
    DateTime date;
    
    bool screen;
    int appNum;
    App *app;
    
    bool getTime(string dateTime) {
      return date.setAllDateTime(dateTime);
    }
};
class CollectionFile {
  // 一個檔案裡的所有資料 之後會用 CollectionAllData 在整個整合起來
  public :
    string phoneID;
    string fileName;
    DateTime date;
    list<Point> pattern;
    vector<string> appNameVec;
    
    // 開啟檔案，並開始讀檔
    bool openFileAndRead() {
      // 開檔部分
      FILE *file;
      file = fopen(fileName.c_str(),"r"); // "r" 讀檔而已
      if(file == NULL) {
        cout << "open \"" << fileName << "\" File fail" << endl ;
        return false;
      }
      
      int line=0;
      //int getLineSize = 4096;
      //int getLineSize = 65536;
      int getLineSize = 262144;
      char getLine[getLineSize];
      
      // 取得 phoneID 以及讀到 "----------" 的話就先往下
      // phoneID:fb5e43235974561d
      while (true) {
        if (fgets(getLine, getLineSize, file) != NULL) {  // 讀一行
          line++;
          if (strstr(getLine, "----------") != NULL) {
            break;
          }
          char* temp = strstr(getLine, "phoneID:");
          if (temp != NULL) {
            phoneID = string(temp+8);
          }
        } else {
          fclose(file);
          return false;
        }
      }
      
      Point lastPoint;
      while (true) { // get Point loop
        Point thisPoint;
        bool isBigScan;
        // 接下來是固定格式 1.時間 2.ProcessRecord 3.procNum 4.AppData
        // 讀取 時間 2018-02-24_00.49.27
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          // 檢查是不是沒有資料了
          if (strstr(getLine,"over")) {
            cout << "fileName:" << fileName << " over data" <<endl;
            break;
          }
          if (!thisPoint.getTime(string(getLine))) {
            cout << "(error) CollectionFile::openFileAndRead() time : " << getLine <<endl;
            break;
          }
        } else {
          break;
        }
        
        // 讀取 什麼類型的搜尋 ProcessRecord:Big or Small
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          // 檢查類型 "Big" or "Small"
          if (strstr(getLine,"Big") != NULL) {
            isBigScan = true;
          } else if (strstr(getLine,"Small") != NULL) {
            isBigScan = false;
          } else {
            cout << "(error) CollectionFile::openFileAndRead() ProcessRecord: " << getLine <<endl;
            break;
          }
        } else {
          break;
        }
        
        // 讀取 APP 數量 procNum:19
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          string tempStr(strstr(getLine,"procNum:"));
          int tempN;
          if (StringToNumber(tempStr.substr(8,tempStr.size() - 9), &tempN)) {
            thisPoint.appNum = tempN;
          } else {
            cout << "(error) CollectionFile::openFileAndRead() procNum: " << getLine <<endl;
            break;
          }
        } else {
          break;
        }
        
        bool appDataGood = true;
        thisPoint.app = new Point::App[thisPoint.appNum];
        // 讀取 APP 詳細資料 並且會依照 Big or Small 有不同的處理方式
        for (int getAppNum=0; getAppNum<thisPoint.appNum; getAppNum++) {
          if (fgets(getLine, getLineSize, file) != NULL) {
            line++;
            Point::App tempApp;
            int index = 0;
            // Big collection
            // name pid 要從中取出來
            if (isBigScan) {
              // 取得 name
              string appName = subCharArray(getLine, getLineSize, '|', index++);
              if (appName.size()!=0) {
                // 判斷 appName 在 appNameVec 中是第幾個 最後給 tempApp.namePoint
                int appNamePoint = 0;
                bool isFindName = false;
                for (int i=0; i < appNameVec.size(); i++) {
                  if (appNameVec[i] != appName) {
                    appNamePoint++;
                  } else {
                    isFindName = true;
                    break;
                  }
                }
                tempApp.namePoint = appNamePoint;
                // 沒有的話 往後面加一個
                if (!isFindName) {
                  appNameVec.push_back(appName);
                }
              } else {
                cout << "(error) CollectionFile::openFileAndRead() appName\n" << getLine <<endl;
                appDataGood = false;
                break;
              }
              
              // 取得 pid
              //cout << "index:" << index <<endl;
              string appPid = subCharArray(getLine, getLineSize, '|', index++);
              if (appPid.size()!=0) {
                if (!StringToNumber(appPid, &tempApp.pid)) {
                  cout << "(error) CollectionFile::openFileAndRead() appPid: " << appPid <<endl;
                  appDataGood = false;
                }
              } else {
                cout << "(error) CollectionFile::openFileAndRead() appPid:NULL \n" << getLine <<endl;
                appDataGood = false;
                break;
              }
            } else { // 從 lastPoint 中 取 name pid 出來給 tempApp
              tempApp.namePoint = lastPoint.app[getAppNum].namePoint;
              tempApp.pid = lastPoint.app[getAppNum].pid;
            }
            
            // 找 TotalPss
            string appTotalPss = subCharArray(getLine, getLineSize, '|', index++);
            if (appTotalPss.size()!=0) {
              if (!StringToNumber(appTotalPss, &tempApp.totalPss)) {
                // 可能只是 "沒有取得資料"
                if (appTotalPss == "null") {
                  tempApp.totalPss = -1;
                } else {
                  cout << "(error) CollectionFile::openFileAndRead() appTotalPss: " << appTotalPss <<endl;
                  appDataGood = false;
                }
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appTotalPss:NULL \n" << getLine <<endl;
              appDataGood = false;
              break;
            }
            
            // 找 oom_score
            string appOomScore = subCharArray(getLine, getLineSize, '|', index++);
            if (appOomScore.size()!=0) {
              if (!StringToNumber(appOomScore, &tempApp.oom_score)) {
                tempApp.oom_score = -10000;
                cout << "(error) CollectionFile::openFileAndRead() appOomScore: " << appOomScore <<endl;
                appDataGood = false;
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appOomScore:NULL \n" << getLine <<endl;
              appDataGood = false;
              break;
            }
            
            // 找 ground
            string appGround = subCharArray(getLine, getLineSize, '|', index++);
            if (appGround.size()!=0) {
              if (!StringToNumber(appGround, &tempApp.ground)) {
                tempApp.ground = -1;
                cout << "(error) CollectionFile::openFileAndRead() appGround: " << appGround <<endl;
                appDataGood = false;
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appGround:NULL \n" << getLine <<endl;
              appDataGood = false;
              break;
            }
            
            // 找 oom_adj
            string appOomAdj = subCharArray(getLine, getLineSize, '|', index++);
            if (appOomAdj.size()!=0) {
              if (!StringToNumber(appOomAdj, &tempApp.oom_adj)) {
                tempApp.oom_adj = -10000;
                cout << "(error) CollectionFile::openFileAndRead() appOomAdj: " << appOomAdj <<endl;
                appDataGood = false;
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appOomAdj:NULL \n" << getLine <<endl;
              appDataGood = false;
              break;
            }
            
            // 將 APP 放入 OneShot 中
            thisPoint.app[getAppNum] = tempApp;
          } else {
            appDataGood = false;
            cout << "(error) CollectionFile::openFileAndRead() procNum not enough"<<endl;
            printf("getAppNum(%d) appNum(%d)", getAppNum, thisPoint.appNum);
            break;
          }
        } // 讀取 APP 詳細資料 loop
        
        /** 接下來為其他資料的讀取
         * 主要是將 ':' 符號前的資料抓出來比對
         * 並且做相對應的事情
         */
        
        while (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          // 發現和 '----------' 一樣的話就換下一筆資料
          if (strstr(getLine,"----------") != NULL) {
            break;
          }
          // 抓出指標物
          string indicate = subCharArray(getLine, getLineSize, ':', 0);
          if (indicate == "Screen") {
            string screen(getLine + 7);
            if (screen.substr(0,2) == "On") {
              thisPoint.screen = true;
            } else if (screen.substr(0,3) == "Off") {
              thisPoint.screen = false;
            } else {
              cout << "(error) CollectionFile::openFileAndRead() Screen:"<< screen <<endl;
              appDataGood = false;
            }
          } else if (indicate == "Battery") { // ===================== bug
            
          } else if (indicate == "WiFi") {
            
          } else if (indicate == "G-sensor") {
            // attention:G-sensor 非常長 目前最長有看到 32645的
          } else if (indicate == "Location") {
            
          } else if (indicate == "WiFiSensor") {
            
          } else if (indicate == "WiFiConnect") {
            // 後面還有資料
            if (fgets(getLine, getLineSize, file) != NULL) {
              line++;
            } else {
              break;
            }
          } else if (indicate == "scanTime") {
            
          } else { // 都沒有抓出的話
            cout << "fileName:" << fileName <<endl;
            cout << "(error) CollectionFile::openFileAndRead() indicate:"<< indicate << "fileName:" << fileName << " line:" << line <<endl;
          }
        }
        lastPoint = thisPoint;
        pattern.push_back(thisPoint);
      } // get Point loop
      
      fclose(file);
      return true;
    };

    // 取得日期的 function
    bool setAllDateTime() {
      // 事先設定好 fileName 可以呼叫這個
      return date.setAllDateTime(fileName);
    };
    bool setAllDateTime(string fileName) {
      this->fileName = fileName;
      return date.setAllDateTime(fileName);
    };
    
    // 比較的是時間的新舊，1:2:4 會大於 1:2:3
    bool operator > (const CollectionFile &otherFile)const {
      return date > otherFile.date;
    };
    bool operator < (const CollectionFile &otherFile)const {
      return date < otherFile.date;
    };
    
  private:
    string subCharArray(const char* charArray, int size, char subUnit, int whichData) {
      if (whichData<0)  // 防呆
        return string("");
      
      int head = 0;
      int end = 0;
      for (int i=0; i<size; i++) {
        if (charArray[i] == subUnit) {
          whichData--;
          if (whichData == 0) { // 找到頭了
            head = i+1;
          } else if (whichData == -1) { // 確定找完了
            end = i;
            if (head == end)  // 怕裡面沒有資料
              return string("");
            
            int size = end - head + 1;
            char temp[size];
            for (int j=0; j<size-1; j++) {
              temp[j] = charArray[head+j];
            }
            temp[size-1] = '\0';
            
            return string(temp);
          }
        }
      }
      
      return string("");
    }
};
#endif /* COLLECTION_FILE_HPP */