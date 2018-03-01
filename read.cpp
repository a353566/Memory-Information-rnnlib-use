#include <dirent.h>
#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <netcdfcpp.h>

#include "tool/DateTime.hpp"
#include "tool/StringToNumber.hpp"
#include "tool/CollectionFile.hpp"
using namespace std;

int getdir(string dir, vector<string> &files);   // 取得資料夾中檔案的方法

class Event {
  public :
    class Case {
      public :
        int namePoint;
        bool isCreat;   // true 的話 只有 nextApp 有東西
        Point::App *thisApp;
        Point::App *nextApp;
        Case() {
          isCreat = false;
        };
    };
    
    DateTime *thisDate;
    DateTime *nextDate;
    vector<Case> caseVec;
    bool isThisScreenOn;
    bool isNextScreenOn;
    Event() {
      isThisScreenOn = false;
      isNextScreenOn = false;
    }
};

class CollectionAllData {
  public :
    // 單一APP的詳細資料，主要是用於分析，對NC檔沒有幫助
    class AppDetail {
      public :
        string appName;
        int findTimes;
        int findRate;
        int oom_adjStat[41]; // 從-20~20 (不過目前只有-17~16)
        
        AppDetail(string appName) {
          this->appName = appName;
          findTimes = 0;
          findRate = 0;
          for (int i=0; i<41; i++) {
            oom_adjStat[i]=0;
          }
        };
        
        bool addOom_adjStat(int oom_adj) {
          if ( -17<=oom_adj && oom_adj<=16 ) {
            oom_adjStat[oom_adj+20]++;
            return true;
          } else if ( -20<=oom_adj && oom_adj<=20 ) {
            oom_adjStat[oom_adj+20]++;
            cout << "oom_adj out of value : " << oom_adj << endl;
            return true;
          } else {
            cout << "oom_adj out of value : " << oom_adj << endl;
            return false;
          }
        };
        int getOom_adjStat(int oom_adj) {
          if ( -20<=oom_adj && oom_adj<=20 ) {
            return oom_adjStat[oom_adj+20];
          } else {
            return -1;
          }
        };
    };
    
    DateTime beginDate;
    DateTime endDate;
    vector<string> allAppNameVec;       // 用 collecFileVec 的資料收集所有的 app's name
    vector<Point> allPatternVec;        // 所有 pattern
    vector<AppDetail> allAppDetailVec;  // 所有 app 的詳細資料
    vector<Event> allEventVec;          // 先都裝到這裡 allEventVec
    
    CollectionAllData() {
    };
    
    void collection(vector<CollectionFile> *collecFileVec) {
      // 收集所有的 app's name 用來之後編號
      for (int i=0; i<collecFileVec->size(); i++) {
        for(list<Point>::iterator oneShot = (*collecFileVec)[i].pattern.begin();
            oneShot!=(*collecFileVec)[i].pattern.end(); oneShot++) 
        {
          addonePoint(*oneShot, &(*collecFileVec)[i].appNameVec);
        }
      }
      // 日期 開始與結束
      beginDate = allPatternVec.front().date;
      endDate = allPatternVec.back().date;
      
      // 從 allPatternVec 中統計 app 的詳細資料
      for (int i=0; i<allAppNameVec.size(); i++) {
        // 新增 AppDetail 並放入名字
        AppDetail newAppDetail(allAppNameVec[i]);
        for (int j=0; j<allPatternVec.size(); j++) {
          // 先找此 oneShot 有沒有此 App
          for (int k=0; k<allPatternVec[j].appNum; k++) {
            // point 一樣的話代表有找到，並記錄相關資料
            if (i==allPatternVec[j].app[k].namePoint) {
              // 紀錄找到了幾次 AND oom_adj 數量
              newAppDetail.findTimes++;
              newAppDetail.addOom_adjStat(allPatternVec[j].app[k].oom_adj);
              break;
            }
          }
        }
        double findRate = ((double)newAppDetail.findTimes)/allPatternVec.size();
        newAppDetail.findRate = static_cast<int>(100*findRate);
        
        // 最後放入 allAppDetailVec
        allAppDetailVec.push_back(newAppDetail);
      }
      
      // 整理 App 的詳細資料
      makeAppDetail();
      
    }
    
    void makeAppDetail() {
      // 有些不需要的直接跳過的 app
      bool *notNeedApp = new bool[allAppDetailVec.size()];
      // 檢查
      for (int i=0; i<allAppDetailVec.size(); i++) {
        // notNeedApp 初始化
        notNeedApp[i] = false;
        // oom_adj 都沒有等於0的資料
        // 表示沒有在前景出現過
        if (allAppDetailVec[i].getOom_adjStat(0)==0) {
          // 目前是直接去除
          notNeedApp[i] = true;
          continue;
        }
        // 過慮自己的收集 App
        int temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:remote",0);
        if(0<=temp) {
          notNeedApp[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo",0);
        if(0<=temp) {
          notNeedApp[i] = true;
          continue;
        }
      }
      
      // 看有沒有0以下的數值
      /*{
        bool isHave = false;
        for (int i=0; i<allAppDetailVec.size(); i++) {
          for (int j=-20; j<0; j++) {
            if (allAppDetailVec[i].getOom_adjStat(j)!=0) {
              if (!isHave) {
                cout << "    =============== oom_adj <0 ================" <<endl;
                cout << "   i  oom_adj  Times  Name " <<endl;
                isHave = true;
              }
              printf("%4d %8d  %5d  %s\n", i, j, allAppDetailVec[i].getOom_adjStat(j), allAppDetailVec[i].appName.c_str());
            }
          }
        }
      }*/
      
      // 找 oom_adj 零以上的資訊
      /*{
        cout << "    ============== 0 <= oom_adj ==============" <<endl;
        printf("  i findRate(%%)");
        for (int j=0; j<=16; j++) {
          printf(" %2d ", j);
        }
        cout << "(%) App's name" <<endl;
        
        // 主要輸出 app Detail 前面會先有過濾的動作，可以只看到想看到的訊息
        for (int i=0; i<allAppDetailVec.size(); i++) {
          // 先看看是不是要跳過的
          if (notNeedApp[i]) {
            continue;
          }
          
          // 看名字中有沒有 ":"
          int temp = allAppDetailVec[i].appName.find(":", 0);
          if (temp<0) {
            continue;
          }
          
          printf("%3d|       %3d|", i, allAppDetailVec[i].findRate);
          for (int j=0; j<=16; j++) {
            if (allAppDetailVec[i].getOom_adjStat(j)==0) {
              printf("   |");
              continue;
            }
            
            double oom_adjRate = ((double)allAppDetailVec[i].getOom_adjStat(j))/allAppDetailVec[i].findTimes;
            printf("%3.0lf|", 100*oom_adjRate);
            //if (j==3 && allAppDetailVec[i].getOom_adjStat(j)!=0) {
            //  printf("%3d%4d%%", i, allAppDetailVec[i].findRate);
            //  cout << " : " << allAppDetailVec[i].appName <<endl;
            //}
          }
          cout << "   |" << allAppDetailVec[i].appName <<endl;
        }
      }*/
      
      // 將 allPatternVec 整理後裝到這裡 allEventVec
      // 查看 oom_adj 發生改變的狀況 allPatternVec
      int changeTimes = 0;
      for (int i=0; i+1<allPatternVec.size(); i++) {
        Event thisEvent;
        
        bool isChange = false;
        bool isOom_adjChToZero = false;
        
        Point::App *thisApp = allPatternVec[i].app;
        int thisAppNum = allPatternVec[i].appNum;
        Point::App *nextApp = allPatternVec[i+1].app;
        int nextAppNum = allPatternVec[i+1].appNum;
        bool isNextAppMatch[nextAppNum];    // 看是不是所有的下一個App都有對應到
        for (int j=0; j<nextAppNum; j++) {
          // 順便過濾
          if (notNeedApp[nextApp[j].namePoint]) {
            isNextAppMatch[j] = true;
          } else {
            isNextAppMatch[j] = false;
          }
        }
        // 從這一個往下連接，一一找一樣的 App
        for (int j=0; j<thisAppNum; j++) {
          // 過濾一些不需要檢查的 app
          if (notNeedApp[thisApp[j].namePoint]) {
            continue;
          }
          
          bool isFindApp = false;
          for (int k=0; k<nextAppNum; k++) {
            if (isNextAppMatch[k]) {
              continue;
            }
            // namePoint 一樣的話，代表示同的 App
            if (thisApp[j].namePoint == nextApp[k].namePoint) {
              isFindApp = true;
              isNextAppMatch[k] = true;
              // 看 oom_adj有沒有變化
              if (thisApp[j].oom_adj != nextApp[k].oom_adj) {
                // 檢查 oom_adj 下一刻變0的狀況
                if (nextApp[k].oom_adj == 0) {
                  isOom_adjChToZero = true;
                  // 收集此case
                  Event::Case thisCase;
                  thisCase.namePoint = thisApp[j].namePoint;
                  thisCase.isCreat = false;
                  thisCase.thisApp = &thisApp[j];
                  thisCase.nextApp = &nextApp[k];
                  thisEvent.caseVec.push_back(thisCase);
                }
              }
              continue;
            }
          }
          if (!isFindApp) {
            isChange = true;
          }
        }
        // 看有沒有新的 app 被單獨創出來，而且還是0
        bool isCreatNewApp = false;
        for (int j=0; j<nextAppNum; j++) {
          if (!isNextAppMatch[j]) {
            if (nextApp[j].oom_adj==0) {
              isCreatNewApp = true;
              // 收集此case
              Event::Case thisCase;
              thisCase.namePoint = nextApp[j].namePoint;
              thisCase.isCreat = true;
              thisCase.nextApp = &nextApp[j];
              thisEvent.caseVec.push_back(thisCase);
            }
          }
        }
        
        // 有改變的話 changeTimes++
        if (isChange || isCreatNewApp) {
          changeTimes++;
        }
        
        // 看有沒有螢幕開關，但沒有其他變化
        bool isScreenChange = (allPatternVec[i].screen != allPatternVec[i+1].screen);
        // 看 oom_adj 有沒有變化 或是有新開的APP 以及螢幕是否有改變
        if (isOom_adjChToZero || isCreatNewApp || isScreenChange) {
          // 都有的話將資料寫入
          thisEvent.thisDate = &allPatternVec[i].date;
          thisEvent.nextDate = &allPatternVec[i+1].date;
          thisEvent.isThisScreenOn = allPatternVec[i].screen;
          thisEvent.isNextScreenOn = allPatternVec[i+1].screen;
          allEventVec.push_back(thisEvent);
        }
      }
      
      // 輸出 oom_adj 變化
      {
        cout << "    ================= oom_adj =================" <<endl;
        cout << "Change Times : " << changeTimes <<endl;
        cout << "oom_adj change(creat) to zero times : " << allEventVec.size() <<endl;
        
        cout << "oom_adj change detail : " <<endl;
        cout << "   No    1    2    3    4    5    6    7    8  other" <<endl;
        int chToZero[10] = {0};
        for (int i=0; i<allEventVec.size(); i++) {
          if (allEventVec[i].caseVec.size()<9)
            chToZero[allEventVec[i].caseVec.size()]++;
          else
            chToZero[9]++;
        }
        for (int i=0; i<10; i++) {
          printf("%5d", chToZero[i]);
        }
        printf("\n");
      }
      // 觀察間隔時間 超過此時間(maxIntervalTime) 將會顯示
      /*{
        DateTime maxIntervalTime;
        maxIntervalTime.initial();
        maxIntervalTime.minute = 5;
        int maxIntervalTimeNum = 0;
        for (int i=0; i<allEventVec.size(); i++) {
          DateTime intervalTime = *allEventVec[i].nextDate - *allEventVec[i].thisDate;
          if (intervalTime>maxIntervalTime) {
            // 有必要在輸出
            if (maxIntervalTimeNum == 0) {
              cout << "    ============== Interval Time ==============" <<endl;
              cout << "Interval Time over ";
              maxIntervalTime.output();
              cout << endl;
              cout << "  #:    i   interval time  this time              next time           screen state             " <<endl;
            }
            printf("%3d:", ++maxIntervalTimeNum);
            printf("%5d :   ", i);
            intervalTime.output();
            printf("     ");
            allEventVec[i].thisDate->output();
            printf(" -> ");
            allEventVec[i].nextDate->output();
            
            //printf(" screen state : ");
            printf(" %s -> " ,allEventVec[i].isThisScreenOn ? " on":"off");
            printf(" %s" ,allEventVec[i].isNextScreenOn ? " on":"off");
            
            printf("\n");
          }
        }
      }*/
    }
    
    void addonePoint(Point oneShot, const vector<string> *oneAppNameVec) {
      for (int i=0; i<oneShot.appNum; i++) {
        findAppNamePoint(&oneShot.app[i], &(*oneAppNameVec)[oneShot.app[i].namePoint]);
      }
      allPatternVec.push_back(oneShot);
    }
    
    int findAppNamePoint(Point::App *app, const string *appName) {
      // 先檢查 allAppNameVec 中有沒有此名字
      for (int i=0; i<allAppNameVec.size(); i++) {
        // 有找到的話直接回傳新的 namePoint
        if (*appName == allAppNameVec[i]) {
          app->namePoint = i;
          return i;
        }
      }
      
      // 沒找到的話則加進去，並取得新的 Point
      allAppNameVec.push_back(*appName);
      app->namePoint = allAppNameVec.size()-1;
      return app->namePoint;
    }
};

class NetCDFoutput {
  public :
    class OneShut {
      public :
        static const int SCREEN_BEGIN = 0;
        static const int SCREEN_LENGTH = 4; // on off off->on on->off
        static const int WEEKDAY_BEGIN = SCREEN_BEGIN + SCREEN_LENGTH;
        static const int WEEKDAY_LENGTH = 7; // 日一二三四五六
        static const int DATETIME_BEGIN = WEEKDAY_BEGIN + WEEKDAY_LENGTH;
        static const int DATETIME_LENGTH = 48; // 24 * 2
        static const int APPDATA_BEGIN = DATETIME_BEGIN + DATETIME_LENGTH;
        
        DateTime *startDate;
        DateTime *endDate;
        // ================= 設定bug ===================
        vector<Point::App*> targetAppVec;
        float *ncData;
        
        OneShut(Event *event) { 
          // 基本資料
          startDate = event->thisDate;
          endDate = event->nextDate;
          for (int i=0; i<event->caseVec.size() ; i++) {
            targetAppVec.push_back(event->caseVec[i].nextApp);
          }
        };
        
        static int getDateTimeCount(DateTime *dateTime) {
          DateTime tempDT = *dateTime;
          // 從 0:30:0 開始數
          tempDT.hour = 0;
          tempDT.minute = 30;
          tempDT.second = 0;
          for (int i=0; i<DATETIME_LENGTH; i++) {
            if (tempDT < *dateTime) {
              // 還沒到 往下加 0:30:0
              if (tempDT.minute == 30) {
                tempDT.hour++;
                tempDT.minute = 0;
              } else {
                tempDT.minute = 30;
              }
            } else {
              return i;
            }
          }
        }
    };
  
    DateTime oneLearningTimeLong; // 一個學習資料的時間長度 (目前為24小時)
    vector<string> *allAppNameVec;
    vector<string> appNameVec;
    vector<Event> *eventVec;
    vector<OneShut> pattern;      // 將 eventVec 轉換完後
    int oneNcDataLength;
    
    int numSeqs;              //    pattern量
    int numTimesteps;         //    全部的資料量
    int inputPattSize;        // OK 輸入資料的維度 (RGB:3)
    int numDims;              // OK 單筆資料的維度 (圖片:2)
    int numLabels;            //    有多少個不同的 app 數量
    int maxLabelLength;       //    App's name 的最長長度
    int maxTargStringLength;  //    targetLenghtVec 中的最大值
    
    NetCDFoutput() {
      // 設定 oneLearningTimeLong (目前為24小時)
      oneLearningTimeLong.initial();
      oneLearningTimeLong.hour = 24;
      
      inputPattSize = 1;
      numDims = 1;      
    }
    
    void collection(vector<Event> *eventVec, vector<string> *allAppNameVec) {
      this->eventVec = eventVec;
      this->allAppNameVec = allAppNameVec;
      // namePoint 指到 appNameTranslate 的位置就是在 nc 檔中的新位置
      int appNameTranslate[allAppNameVec->size()];
      int appNum = 0;
      for (int i=0; i<allAppNameVec->size(); i++) {
        appNameTranslate[i] = -1;
      }
      
      // 將 appNameTranslate 建立起來
      // 收尋所有 app
      for (int i=0; i<eventVec->size(); i++) {
        for (int j=0; j<(*eventVec)[i].caseVec.size(); j++) {
          // 沒有在 appNameTranslate 找到的話(等於-1)，就是沒有
          int oldNamePoint = (*eventVec)[i].caseVec[j].namePoint;
          if (appNameTranslate[oldNamePoint] == -1) {
            appNameTranslate[oldNamePoint] = appNum;
            appNum++;
            // 建立新的 appNameVec
            appNameVec.push_back((*allAppNameVec)[oldNamePoint]);
          }
        }
      }
      
      oneNcDataLength = OneShut::APPDATA_BEGIN + appNum;
      for (int i=0; i<eventVec->size(); i++) {
        // 宣告 並給予 &(*eventVec)[i] 讓他可以建立基本資料
        OneShut oneShut(&(*eventVec)[i]);
        oneShut.ncData = new float[oneNcDataLength];
        for (int j=0; j<oneNcDataLength; j++) {
          oneShut.ncData[j] = 0;
        }
        
        // screen
        if ((*eventVec)[i].isNextScreenOn) {
          oneShut.ncData[OneShut::SCREEN_BEGIN + 0] = 1;
          if (!(*eventVec)[i].isThisScreenOn) {
            oneShut.ncData[OneShut::SCREEN_BEGIN + 2] = 1;
          }
        } else {
          oneShut.ncData[OneShut::SCREEN_BEGIN + 1] = 1;
          if ((*eventVec)[i].isThisScreenOn) {
            oneShut.ncData[OneShut::SCREEN_BEGIN + 3] = 1;
          }
        }
        
        // weekday
        int weekday = (*eventVec)[i].nextDate->WeekDay();
        oneShut.ncData[OneShut::WEEKDAY_BEGIN + weekday] = 1;
        
        // timeCount
        int timeCount = OneShut::getDateTimeCount((*eventVec)[i].nextDate);
        oneShut.ncData[OneShut::DATETIME_BEGIN + timeCount] = 1;
        
        // appData
        for (int j=0; j<(*eventVec)[i].caseVec.size(); j++) {
          int newNamePoint = appNameTranslate[(*eventVec)[i].caseVec[j].namePoint];
          if (newNamePoint<0 || appNum<=newNamePoint) { // 不在 0 ~ appNum 範圍內
            cout << "(error) newNamePoint(" << newNamePoint << ") no between 0 and " << appNum << "." <<endl;
          }
          oneShut.ncData[OneShut::APPDATA_BEGIN + newNamePoint] = 1;
        }
        pattern.push_back(oneShut);
      }
      
      cout << "    ================ nc output ================" <<endl;
      cout << "從 " << allAppNameVec->size() << " 刪到剩 " << appNum << " 個有用到的 app" <<endl;
      
      // pattern ncData 測試
      for (int i=0; i<pattern.size(); i++) {
        for (int j=0; j<oneNcDataLength; j++) {
          cout << pattern[i].ncData[j];
        }
        cout <<endl;
      }
      
      ncOutput2();
    }
    
    void ncOutput() {
      // 找合格的有多少個 並分析
      vector<int> inputsLenghtVec;
      vector<char*> inputsStrVec;
      vector<int> targetLenghtVec;
      vector<char*> targetStrVec;
      int head = 0, end = 1;
      DateTime headTime = *pattern[head].endDate;
      
      { // 看有沒有時間超過的太誇張
        bool isHave = false;
        DateTime intervalTime;
        intervalTime.initial();
        intervalTime.hour = 8;
        for (int i=1; i<pattern.size(); i++) {
          if (*pattern[i].endDate - *pattern[i-1].endDate > intervalTime) {
            if (!isHave) {
              isHave = true;
              cout << "    ========= Interval Time > 8 hour ==========" <<endl;
              cout << "              start                     end     interval" <<endl;
            }
            pattern[i-1].endDate->output();
            cout << "\t";
            pattern[i].endDate->output();
            cout << "\t";
            (*pattern[i].endDate - *pattern[i-1].endDate).output();
            cout << endl;
          }
        }
      }
      
      for (int i=1; i<pattern.size(); i++) {
        DateTime endTime = *pattern[i].endDate;
        // 如果超過可以學習的時間長度
        if (endTime - headTime > oneLearningTimeLong) {
          // 就先看看當下的 OneShut 有沒有 creat app
          if (pattern[i].targetAppVec.size() > 0) {
            // 有的話 就可以當作一筆資料
            // 先找最接近 oneLearningTimeLong 的時間段
            do {
              head++;
              headTime = *pattern[head].endDate;
            }
            while (endTime - headTime > oneLearningTimeLong);
            
            // 如果找到同一個時間點的話， 代表上個點到這個點之前超過 oneLearningTimeLong 時間
            if (i <= head) {
              // 有問題，且資料不能用，並跳過此點的數據
              cout << "(error) NetCDFoutput::ncOutput() i=" << i << " head=" << head <<endl;
              continue;
            }
            
            // 讓 head 往上跳一格
            head--;
            headTime = *pattern[head].endDate;
            
            // 開始記錄相關資訊
            // input 部分
            int inputsLenght = (i-head) * oneNcDataLength;  // 長度
            char *inputChars = new char[inputsLenght];                  // 內容
            int count=0;
            for (int j=head; j<i; j++) {
              for (int k=0; k<oneNcDataLength; k++) {
                inputChars[count++] = pattern[j].ncData[k];
              }
            }
            inputsLenghtVec.push_back(inputsLenght);
            inputsStrVec.push_back(inputChars);
            
            // target 部分
            // 要先知道長度
            int targetLenght = 0; // 長度
            count = 0;
            for (int j=0; j<pattern[i].targetAppVec.size(); j++) {
              targetLenght += 1 + (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].size();
            }
            char *targetChars = new char[targetLenght]; // 內容
            for (int j=0; j<pattern[i].targetAppVec.size(); j++) {
              int c_strSize = (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].size();
              const char *appName = (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].c_str();
              for (int k=0; k<c_strSize; k++) {
                targetChars[count++] = appName[k];
              }
              targetChars[count++] = ' ';
            }
            targetChars[count-1] = '\0';
            targetLenghtVec.push_back(targetLenght);
            targetStrVec.push_back(targetChars);
          }
        }
      }
      
      // =======================================
      
      // ---------- numSeqs maxTargStringLength
      numSeqs = inputsStrVec.size();  // pattern量
      maxTargStringLength = 0;        // targetLenghtVec 中的最大值
      for (int i=0; i<targetLenghtVec.size(); i++) {
        if (maxTargStringLength < targetLenghtVec[i]) {
          maxTargStringLength = targetLenghtVec[i];
        }
      }
      cout << "Pattern Number : " << numSeqs <<endl;
      cout << "Max Target String Length : " << maxTargStringLength <<endl;
      
      
      // ---------- numLabels maxLabelLength
      numLabels = appNameVec.size(); // 有多少個不同的 app 數量
      maxLabelLength = 0;            // App's name 的最長長度
      for (int i=0; i<appNameVec.size(); i++) {
        if (maxLabelLength<appNameVec[i].size()) {
          maxLabelLength = appNameVec[i].size();
        }
      }
      
      // ============== 轉成 nc 檔 ==============
      NcFile dataFile(getNcFileName(TRAINING_FILE), NcFile::Replace);
      if (!dataFile.is_valid()) { // 看能不能用
        cout << "(error)NetCDFoutput::ncOutput() Couldn't open \"" << getNcFileName(TRAINING_FILE) << "\" file!";
        return;
      }
      
      //  ===== char labels(numLabels, maxLabelLength) 
      {
        maxLabelLength++; // 最後一位要給 '\0'
        NcDim *numLabelsNC = dataFile.add_dim("numLabels", numLabels);
        NcDim *maxLabelLengthNC = dataFile.add_dim("maxLabelLength", maxLabelLength);
        NcVar *labelsNC = dataFile.add_var("labels", ncChar, numLabelsNC, maxLabelLengthNC);
        // 整理檔案 (labels)
        // vector<string> appNameVec;
        for(int i=0; i<numLabels; i++) {
          int lenght = appNameVec[i].size();
          char appName[maxLabelLength];
          const char *temp = appNameVec[i].c_str();
          for (int j=0; j<maxLabelLength; j++) {
            appName[j] = (j<lenght) ? temp[j] : '\0';
          }
          // 一個一個放入
          labelsNC->put_rec(numLabelsNC, appName, i);
        }
      }
      
      // ===== char targetStrings(numSeqs, maxTargStringLength)
      {
        NcDim *numSeqsNC = dataFile.add_dim("numSeqs", numSeqs);
        NcDim *maxTargStringLengthNC = dataFile.add_dim("maxTargStringLength", maxTargStringLength);
        NcVar *targetStringsNC = dataFile.add_var("targetStrings", ncChar, numSeqsNC, maxTargStringLengthNC);
        // 整理檔案 (targetStrings)
        //   vector<int> targetLenghtVec;
        //   vector<char*> targetStrVec;
        for(int i=0; i<numSeqs; i++) {
          int lenght = targetLenghtVec[i];
          char targetString[maxTargStringLength];
          const char *temp = targetStrVec[i];
          for (int j=0; j<maxTargStringLength; j++) {
            targetString[j] = (j<lenght) ? temp[j] : '\0';
          }
          // 一個一個放入
          targetStringsNC->put_rec(numSeqsNC, targetString, i);
        }
      }
      
      // ===== int seqLengths(numSeqs)
      numTimesteps = 0;  // 順便找全部的資料量
      {
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcVar *seqLengthsNC = dataFile.add_var("seqLengths", ncInt, numSeqsNC);
        //   vector<int> inputsLenghtVec;
        for (int i=0; i<inputsLenghtVec.size(); i++) {
          numTimesteps += inputsLenghtVec[i];
          seqLengthsNC->put_rec(numSeqsNC, &inputsLenghtVec[i], i);
        }
      }
      
      // ===== int seqDims(numSeqs, numDims)
      numDims = 2;  // 單筆資料的維度 (圖片:2)
      {
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *numDimsNC = dataFile.add_dim("numDims", numDims);
        NcVar *seqDimsNC = dataFile.add_var("seqDims", ncInt, numSeqsNC, numDimsNC);
        // 整理檔案 (seqDims)
        //   vector<int> inputsLenghtVec;
        for (int i=0; i<inputsLenghtVec.size(); i++) {
          int temp[numDims];
          oneNcDataLength;
          temp[0] = oneNcDataLength;
          temp[1] = inputsLenghtVec[i] / oneNcDataLength;
          //temp[0] = inputsLenghtVec[i];
          //temp[1] = 1;
          seqDimsNC->put_rec(numSeqsNC, &temp[0], i);
        }
      }
      
      // ===== float inputs(numTimesteps, inputPattSize)
      inputPattSize = 1;  // 輸入資料的維度 (RGB:3)
      {
        NcDim *numTimestepsNC = dataFile.add_dim("numTimesteps", numTimesteps);
        NcDim *inputPattSizeNC = dataFile.add_dim("inputPattSize", inputPattSize);
        NcVar *inputsNC = dataFile.add_var("inputs", ncFloat, numTimestepsNC, inputPattSizeNC);
        // 整理檔案 (inputs)
        //   vector<int> inputsLenghtVec;
        //   vector<char*> inputsStrVec;
        float inputs[numTimesteps][inputPattSize];
        int count = 0;
        for (int i=0; i<inputsLenghtVec.size(); i++) {
          for (int j=0; j<inputsLenghtVec[i]; j++) {
            inputs[count++][0] = inputsStrVec[i][j];
          }
        }
        inputsNC->put(&inputs[0][0], numTimesteps, inputPattSize);
      }
      
      // ===== seqTags(numSeqs, maxSeqTagLength)
      {
        int maxSeqTagLength = 32;
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *maxSeqTagLengthNC = dataFile.add_dim("maxSeqTagLength", maxSeqTagLength);
        NcVar *seqTagsNC = dataFile.add_var("seqTags", ncChar, numSeqsNC, maxSeqTagLengthNC);
        // 整理檔案 (numSeqs)
        for(int i=0; i<numSeqs; i++) {
          char *temp;
          int num = i;
          temp = new char[maxSeqTagLength];
          for (int j=0; j<maxSeqTagLength; j++) {
            temp[j] = '\0';
          }
          sprintf(temp, "%d-%d", num, inputsLenghtVec[i]);
          // 一個一個放入
          seqTagsNC->put_rec(numSeqsNC, &temp[0], i);
        }
      }
      return ;
    };
    
    // 會輸出 10%的Testing檔案
    void ncOutput2() {
      // 找合格的有多少個 並分析
      vector<int> inputsLenghtVec;
      vector<char*> inputsStrVec;
      vector<int> targetLenghtVec;
      vector<char*> targetStrVec;
      int head = 0, end = 1;
      DateTime headTime = *pattern[head].endDate;
      
      { // 看有沒有時間超過的太誇張
        bool isHave = false;
        DateTime intervalTime;
        intervalTime.initial();
        intervalTime.hour = 8;
        for (int i=1; i<pattern.size(); i++) {
          if (*pattern[i].endDate - *pattern[i-1].endDate > intervalTime) {
            if (!isHave) {
              isHave = true;
              cout << "    ========= Interval Time > 8 hour ==========" <<endl;
              cout << "              start                     end     interval" <<endl;
            }
            pattern[i-1].endDate->output();
            cout << "\t";
            pattern[i].endDate->output();
            cout << "\t";
            (*pattern[i].endDate - *pattern[i-1].endDate).output();
            cout << endl;
          }
        }
      }
      
      for (int i=1; i<pattern.size(); i++) {
        DateTime endTime = *pattern[i].endDate;
        // 如果超過可以學習的時間長度
        if (endTime - headTime > oneLearningTimeLong) {
          // 就先看看當下的 OneShut 有沒有 creat app
          if (pattern[i].targetAppVec.size() > 0) {
            // 有的話 就可以當作一筆資料
            // 先找最接近 oneLearningTimeLong 的時間段
            do {
              head++;
              headTime = *pattern[head].endDate;
            }
            while (endTime - headTime > oneLearningTimeLong);
            
            // 如果找到同一個時間點的話， 代表上個點到這個點之前超過 oneLearningTimeLong 時間
            if (i <= head) {
              // 有問題，且資料不能用，並跳過此點的數據
              cout << "(error) NetCDFoutput::ncOutput() i=" << i << " head=" << head <<endl;
              continue;
            }
            
            // 讓 head 往上跳一格
            head--;
            headTime = *pattern[head].endDate;
            
            // 開始記錄相關資訊
            // input 部分
            int inputsLenght = (i-head) * oneNcDataLength;  // 長度
            char *inputChars = new char[inputsLenght];                  // 內容
            int count=0;
            for (int j=head; j<i; j++) {
              for (int k=0; k<oneNcDataLength; k++) {
                inputChars[count++] = pattern[j].ncData[k];
              }
            }
            inputsLenghtVec.push_back(inputsLenght);
            inputsStrVec.push_back(inputChars);
            
            // target 部分
            // 要先知道長度
            int targetLenght = 0; // 長度
            count = 0;
            for (int j=0; j<pattern[i].targetAppVec.size(); j++) {
              targetLenght += 1 + (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].size();
            }
            char *targetChars = new char[targetLenght]; // 內容
            for (int j=0; j<pattern[i].targetAppVec.size(); j++) {
              int c_strSize = (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].size();
              const char *appName = (*allAppNameVec)[pattern[i].targetAppVec[j]->namePoint].c_str();
              for (int k=0; k<c_strSize; k++) {
                targetChars[count++] = appName[k];
              }
              targetChars[count++] = ' ';
            }
            targetChars[count-1] = '\0';
            targetLenghtVec.push_back(targetLenght);
            targetStrVec.push_back(targetChars);
          }
        }
      }
      
      // =======================================
      int testFileLong = 10;  // testFile(%)
      int trainingSize = ((double)(100-testFileLong)/100) * inputsStrVec.size();
      int testingSize = inputsStrVec.size() - trainingSize;
      
      // ============== 轉成 nc 檔 ==============
      NcFile dataFile(getNcFileName(TRAINING_FILE), NcFile::Replace);
      if (!dataFile.is_valid()) { // 看能不能用
        cout << "(error)NetCDFoutput::ncOutput() Couldn't open \"" << getNcFileName(TRAINING_FILE) << "\" file!";
        return;
      }
      NcFile testFile(getNcFileName(TESTING_FILE), NcFile::Replace);
      if (!dataFile.is_valid()) { // 看能不能用
        cout << "(error)NetCDFoutput::ncOutput() Couldn't open \"" << getNcFileName(TESTING_FILE) << "\" file!";
        return;
      }
      
      // ---------- numSeqs maxTargStringLength
      numSeqs = inputsStrVec.size();  // pattern量
      maxTargStringLength = 0;        // targetLenghtVec 中的最大值
      for (int i=0; i<targetLenghtVec.size(); i++) {
        if (maxTargStringLength < targetLenghtVec[i]) {
          maxTargStringLength = targetLenghtVec[i];
        }
      }
      cout << "Pattern Number : " << numSeqs <<endl;
      cout << "Max Target String Length : " << maxTargStringLength <<endl;
      
      // ---------- numLabels maxLabelLength
      numLabels = appNameVec.size(); // 有多少個不同的 app 數量
      maxLabelLength = 0;            // App's name 的最長長度
      for (int i=0; i<appNameVec.size(); i++) {
        if (maxLabelLength<appNameVec[i].size()) {
          maxLabelLength = appNameVec[i].size();
        }
      }
      
      //  ===== char labels(numLabels, maxLabelLength) 
      {
        maxLabelLength++; // 最後一位要給 '\0'
        NcDim *numLabelsNC = dataFile.add_dim("numLabels", numLabels);
        NcDim *maxLabelLengthNC = dataFile.add_dim("maxLabelLength", maxLabelLength);
        NcVar *labelsNC = dataFile.add_var("labels", ncChar, numLabelsNC, maxLabelLengthNC);
        // test
        NcDim *numLabelsNCtest = testFile.add_dim("numLabels", numLabels);
        NcDim *maxLabelLengthNCtest = testFile.add_dim("maxLabelLength", maxLabelLength);
        NcVar *labelsNCtest = testFile.add_var("labels", ncChar, numLabelsNCtest, maxLabelLengthNCtest);
        // 整理檔案 (labels)
        // vector<string> appNameVec;
        for(int i=0; i<numLabels; i++) {
          int lenght = appNameVec[i].size();
          char appName[maxLabelLength];
          const char *temp = appNameVec[i].c_str();
          for (int j=0; j<maxLabelLength; j++) {
            appName[j] = (j<lenght) ? temp[j] : '\0';
          }
          // 一個一個放入 且一起放就好
          labelsNC->put_rec(numLabelsNC, appName, i);
          labelsNCtest->put_rec(numLabelsNCtest, appName, i);
        }
      }
      
      // ===== char targetStrings(numSeqs, maxTargStringLength)
      {
        NcDim *numSeqsNC = dataFile.add_dim("numSeqs", trainingSize);
        NcDim *maxTargStringLengthNC = dataFile.add_dim("maxTargStringLength", maxTargStringLength);
        NcVar *targetStringsNC = dataFile.add_var("targetStrings", ncChar, numSeqsNC, maxTargStringLengthNC);
        // test
        NcDim *numSeqsNCtest = testFile.add_dim("numSeqs", testingSize);
        NcDim *maxTargStringLengthNCtest = testFile.add_dim("maxTargStringLength", maxTargStringLength);
        NcVar *targetStringsNCtest = testFile.add_var("targetStrings", ncChar, numSeqsNCtest, maxTargStringLengthNCtest);
        // 整理檔案 (targetStrings)
        //   vector<int> targetLenghtVec;
        //   vector<char*> targetStrVec;
        for(int i=0; i<numSeqs; i++) {
          int lenght = targetLenghtVec[i];
          char targetString[maxTargStringLength];
          const char *temp = targetStrVec[i];
          for (int j=0; j<maxTargStringLength; j++) {
            targetString[j] = (j<lenght) ? temp[j] : '\0';
          }
          // 一個一個放入 且要分開放
          if (i < trainingSize) {
            targetStringsNC->put_rec(numSeqsNC, targetString, i);
          } else if (i - trainingSize < testingSize) {
            targetStringsNCtest->put_rec(numSeqsNCtest, targetString, i - trainingSize);
          } else {
            cout << "(error)NetCDFoutput::ncOutput2()char targetStrings(...)" <<endl;
            cout << "       i(" << i << ") - trainingSize(" << trainingSize << ") > testingSize(" << testingSize << ")" <<endl;
          }
        }
      }
      
      // ===== int seqLengths(numSeqs)
      numTimesteps = 0;  // 順便找全部的資料量
      int numTimestepsTraining = 0;
      int numTimestepsTesting = 0;
      {
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcVar *seqLengthsNC = dataFile.add_var("seqLengths", ncInt, numSeqsNC);
        // test
        NcDim *numSeqsNCtest = testFile.get_dim("numSeqs");  // 上面已經有了
        NcVar *seqLengthsNCtest = testFile.add_var("seqLengths", ncInt, numSeqsNCtest);
        //   vector<int> inputsLenghtVec;
        for (int i=0; i<numSeqs; i++) {
          numTimesteps += inputsLenghtVec[i];
          // 一個一個放入 且要分開放
          if (i < trainingSize) {
            numTimestepsTraining += inputsLenghtVec[i];
            seqLengthsNC->put_rec(numSeqsNC, &inputsLenghtVec[i], i);
          } else if (i - trainingSize < testingSize) {
            numTimestepsTesting += inputsLenghtVec[i];
            seqLengthsNCtest->put_rec(numSeqsNCtest, &inputsLenghtVec[i], i - trainingSize);
          } else {
            cout << "(error)NetCDFoutput::ncOutput2()int seqLengths(...)" <<endl;
            cout << "       i(" << i << ") - trainingSize(" << trainingSize << ") > testingSize(" << testingSize << ")" <<endl;
          }
        }
      }
      // ===== int seqDims(numSeqs, numDims)
      numDims = 2;  // 單筆資料的維度 (圖片:2)
      {
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *numDimsNC = dataFile.add_dim("numDims", numDims);
        NcVar *seqDimsNC = dataFile.add_var("seqDims", ncInt, numSeqsNC, numDimsNC);
        // test
        NcDim *numSeqsNCtest = testFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *numDimsNCtest = testFile.add_dim("numDims", numDims);
        NcVar *seqDimsNCtest = testFile.add_var("seqDims", ncInt, numSeqsNCtest, numDimsNCtest);
        // 整理檔案 (seqDims)
        //   vector<int> inputsLenghtVec;
        for (int i=0; i<numSeqs; i++) {
          int temp[numDims];
          oneNcDataLength;
          temp[0] = oneNcDataLength;
          temp[1] = inputsLenghtVec[i] / oneNcDataLength;
          // 一個一個放入 且要分開放
          if (i < trainingSize) {
            seqDimsNC->put_rec(numSeqsNC, &temp[0], i);
          } else if (i - trainingSize < testingSize) {
            seqDimsNCtest->put_rec(numSeqsNCtest, &temp[0], i - trainingSize);
          } else {
            cout << "(error)NetCDFoutput::ncOutput2()int seqDims(...)" <<endl;
            cout << "       i(" << i << ") - trainingSize(" << trainingSize << ") > testingSize(" << testingSize << ")" <<endl;
          }
        }
      }
      
      // ===== float inputs(numTimesteps, inputPattSize)
      inputPattSize = 1;  // 輸入資料的維度 (RGB:3)
      {
        NcDim *numTimestepsNC = dataFile.add_dim("numTimesteps", numTimestepsTraining);
        NcDim *inputPattSizeNC = dataFile.add_dim("inputPattSize", inputPattSize);
        NcVar *inputsNC = dataFile.add_var("inputs", ncFloat, numTimestepsNC, inputPattSizeNC);
        //test
        NcDim *numTimestepsNCtest = testFile.add_dim("numTimesteps", numTimestepsTesting);
        NcDim *inputPattSizeNCtest = testFile.add_dim("inputPattSize", inputPattSize);
        NcVar *inputsNCtest = testFile.add_var("inputs", ncFloat, numTimestepsNCtest, inputPattSizeNCtest);
        // 整理檔案 (inputs)
        //   vector<int> inputsLenghtVec;
        //   vector<char*> inputsStrVec;
        float inputsTraining[numTimestepsTraining][inputPattSize];  // 一次宣告全部大小 並且一次放完
        float inputsTesting[numTimestepsTesting][inputPattSize];  // 一次宣告全部大小 並且一次放完
        int countTraining = 0;
        int countTesting = 0;
        for (int i=0; i<inputsLenghtVec.size(); i++) {
          for (int j=0; j<inputsLenghtVec[i]; j++) {
            // 一個一個放入 且要分開放
            if (i < trainingSize) {
              inputsTraining[countTraining++][0] = inputsStrVec[i][j];
            } else if (i - trainingSize < testingSize) {
              inputsTesting[countTesting++][0] = inputsStrVec[i][j];
            } else {
              cout << "(error)NetCDFoutput::ncOutput2()float inputs(...)" <<endl;
              cout << "       i(" << i << ") - trainingSize(" << trainingSize << ") > testingSize(" << testingSize << ")" <<endl;
            }
          }
        }
        // 最後放入
        inputsNC->put(&inputsTraining[0][0], numTimestepsTraining, inputPattSize);
        inputsNCtest->put(&inputsTesting[0][0], numTimestepsTesting, inputPattSize);
      }
      
      // ===== char seqTags(numSeqs, maxSeqTagLength)
      {
        int maxSeqTagLength = 32;
        NcDim *numSeqsNC = dataFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *maxSeqTagLengthNC = dataFile.add_dim("maxSeqTagLength", maxSeqTagLength);
        NcVar *seqTagsNC = dataFile.add_var("seqTags", ncChar, numSeqsNC, maxSeqTagLengthNC);
        // test
        NcDim *numSeqsNCtest = testFile.get_dim("numSeqs");  // 上面已經有了
        NcDim *maxSeqTagLengthNCtest = testFile.add_dim("maxSeqTagLength", maxSeqTagLength);
        NcVar *seqTagsNCtest = testFile.add_var("seqTags", ncChar, numSeqsNCtest, maxSeqTagLengthNCtest);
        // 整理檔案 (numSeqs)
        for(int i=0; i<numSeqs; i++) {
          char *temp;
          int num = i;
          temp = new char[maxSeqTagLength];
          for (int j=0; j<maxSeqTagLength; j++) {
            temp[j] = '\0';
          }
          sprintf(temp, "%d-%d", num, inputsLenghtVec[i]);
          // 一個一個放入
          seqTagsNC->put_rec(numSeqsNC, &temp[0], i);
          
          // 一個一個放入 且要分開放
          if (i < trainingSize) {
            seqTagsNC->put_rec(numSeqsNC, &temp[0], i);
          } else if (i - trainingSize < testingSize) {
            seqTagsNCtest->put_rec(numSeqsNCtest, &temp[0], i - trainingSize);
          } else {
            cout << "(error)NetCDFoutput::ncOutput2()char seqTags(...)" <<endl;
            cout << "       i(" << i << ") - trainingSize(" << trainingSize << ") > testingSize(" << testingSize << ")" <<endl;
          }
        }
      }
      return ;
    };
    
  private :
    static const int TRAINING_FILE = 100;
    static const int TESTING_FILE = 101;
    char* getNcFileName(int mod) {
      const int bufferSize = 64;
      DateTime *startDate = pattern[0].startDate;
      DateTime *endDate = pattern[pattern.size()-1].endDate;
      char *name;
      name = new char[bufferSize];
      if (mod == TRAINING_FILE) {
        sprintf(name, "RNN_Training_%d-%d-%d_to_%d_%d_%d.nc", 
                startDate->year, startDate->month, startDate->day,
                endDate->year, endDate->month, endDate->day);
      } else if (mod == TESTING_FILE) {
        sprintf(name, "RNN_Testing_%d-%d-%d_to_%d_%d_%d.nc", 
                startDate->year, startDate->month, startDate->day,
                endDate->year, endDate->month, endDate->day);
      } else {
        cout << "(error)NetCDFoutput::getNcFileName(int mod) mod=" << mod <<endl;
        return NULL;
      }
      return name;
    };
};

int main(int argc, char** argv) {
  // 資料夾路徑(絕對位址or相對位址) 目前用現在的位置
  
  string dir;
  if (argc == 2) {
    dir = string(argv[1]);
  } else {
    dir = string(".");
  }
  
  vector<string> files;
  vector<CollectionFile> collecFileVec; // 所有檔案的 vector
  CollectionAllData collecAllData;      // 整理所有資料
  NetCDFoutput netCDFoutput;            // 將 collecAllData 整理出來的資料轉 nc 檔
  // 取出檔案，並判斷有沒有問題
  if (getdir(dir, files) == -1) {
    cout << "Error opening" << dir << endl;
    return -1;
  }

  // 整理所有檔案內容
  for(int i=0; i<files.size(); i++) {
    CollectionFile oneFile;
    oneFile.fileName = files[i];
    // 檢查檔案看是不是需要的檔案，像 2017-12-24_03.20.27 就是正確的檔名
	  if (!oneFile.setAllDateTime()) {
      continue;
    }
    oneFile.fileName = dir + oneFile.fileName;
    // 確定沒問題後打開檔案
    if (oneFile.openFileAndRead()) {
      // 檔案資料正確後，放入檔案的 linked list 中
      // 順便排序
      int ins = 0;
      for (ins=0; ins<collecFileVec.size(); ins++) {
        if (collecFileVec[ins] > oneFile)
          break;
      }
      collecFileVec.insert(collecFileVec.begin()+ins, oneFile);
    } else {
      continue;
    }
  }
  // 檔名依序輸出
  //for (int i=0; i<collecFileVec.size(); i++)
  //  collecFileVec[i].date.output();
  
  // 輸出數量
  cout << "收集了多少" << collecFileVec.size() << "檔案" <<endl;
  
  // 將檔案給 collecAllData 整理成可讓 netCDFoutput 讀的資料
  collecAllData.collection(&collecFileVec);
  // 將 collecAllData 中的 allEventVec appNameVec 給 netCDFoutput 去整理
  netCDFoutput.collection(&collecAllData.allEventVec, &collecAllData.allAppNameVec);
  
  cout << "  over" <<endl;
  return 0;
}

int getdir(string dir, vector<string> &files) {
  // 創立資料夾指標
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(dir.c_str())) == NULL) {
    return -1;
  }
  // 如果dirent指標非空
  while((dirp = readdir(dp)) != NULL) {
    // 將資料夾和檔案名放入vector
    files.push_back(string(dirp->d_name));
  }
  // 關閉資料夾指標
  closedir(dp);
  return 0;
}
