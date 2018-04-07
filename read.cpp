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

    // 將重複的 app 刪掉
    bool sortOut() {
      bool dataGood = true;
      for (int i=0; i<int(caseVec.size())-1; i++) {
        int namePoint = caseVec.at(i).namePoint;
        for (int j=i+1; j<caseVec.size(); j++) {
          // 找到同樣 name 的 app
          // 保留 oom_score 比較小的
          // oom_score 一樣的話 就保留 appPid 比較小的
          if (namePoint == caseVec.at(j).namePoint) {
            /*if (dataGood) {
              cout << "-------" << caseVec.size() <<endl;
              for (int k=0; k<caseVec.size(); k++) {
                cout << caseVec.at(k).nextApp->oom_score << " : " << caseVec.at(k).nextApp->namePoint <<endl;
              }
            }*/

            int front = caseVec.at(i).nextApp->oom_score;
            int behind = caseVec.at(j).nextApp->oom_score;
            if (front < behind) {
              caseVec.erase(caseVec.begin()+j);
              j--;
            } else if (front > behind) {
              caseVec.erase(caseVec.begin()+i);
              i--;
              break;
            } else {
              front = caseVec.at(i).nextApp->pid;
              behind = caseVec.at(j).nextApp->pid;
              if (front < behind) {
                caseVec.erase(caseVec.begin()+j);
                j--;
              } else if (front > behind) {
                caseVec.erase(caseVec.begin()+i);
                i--;
                break;
              } else { // 都一樣的話 丟後面的 雖然不太可能發生
                caseVec.erase(caseVec.begin()+j);
                j--;
              }
            }
            dataGood = false;
          }
        }
      }
      /*if (!dataGood) {
        cout << "new--" << caseVec.size() <<endl;
        for (int k=0; k<caseVec.size(); k++) {
          cout << caseVec.at(k).nextApp->oom_score << " : " << caseVec.at(k).nextApp->namePoint <<endl;
        }
      }*/
      return dataGood;
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
    /**
     * allEventVec 主要是放入了 發生過的事件 並依序放好
     * 其中的 Event 是使用者剛好切換 APP 或是開關螢幕也會知道
     */
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

    // 主要將資料裝到 allEventVec
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
        int temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:Record",0);
        if(0<=temp) {
          notNeedApp[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:Recover",0);
        if(0<=temp) {
          notNeedApp[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:remote",0);
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
        // 看 oom_adj 有沒有變成0 或是有新開的APP 以及螢幕是否有改變
        if (isOom_adjChToZero || isCreatNewApp || isScreenChange) {
          // 都有的話將資料寫入
          thisEvent.thisDate = &allPatternVec[i].date;
          thisEvent.nextDate = &allPatternVec[i+1].date;
          thisEvent.isThisScreenOn = allPatternVec[i].screen;
          thisEvent.isNextScreenOn = allPatternVec[i+1].screen;
          allEventVec.push_back(thisEvent);
        }
      }

      // allEventVec 中的 app 可能出現重複的 app (收集時就有問題)
      {
        for (int i=0; i<allEventVec.size(); i++) {
          allEventVec.at(i).sortOut();
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

class DataMining {
  public :
    string screen_turn_on;
    string screen_turn_off;
    string screen_long_off;
    int EPscreen_turn_on;
    int EPscreen_turn_off;
    int EPscreen_long_off;
    DateTime intervalTime;
    vector<int> DMEventPoint;
    vector<string> DMEPtoEvent;
    DataMining() {
      screen_turn_on = string("screen turn on");
      screen_turn_off = string("screen turn off");
      screen_long_off = string("screen long off");
      // 看螢幕"暗著"的時間間隔 // mason
      intervalTime.initial();
      intervalTime.hour = 1;
    }

    bool collection(vector<Event> *eventVec, vector<string> *allAppNameVec) {
      bool lastScreen = eventVec->at(0).isNextScreenOn;
      for (int i=1; i<eventVec->size(); i++) {
        if (lastScreen != eventVec->at(i).isThisScreenOn) {
          cout << "not good" <<endl;
        }
        lastScreen = eventVec->at(i).isNextScreenOn;
        
      }
      // 先將 app 一一列好
      // initial
      // 先放 app name
      for (int i=0; i<allAppNameVec->size(); i++) {
        DMEPtoEvent.push_back(allAppNameVec->at(i));
      }
      // screen "screen turn on" "screen turn off" "screen long off"
      DMEPtoEvent.push_back(screen_turn_on);
      EPscreen_turn_on = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_turn_off);
      EPscreen_turn_off = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_long_off);
      EPscreen_long_off = DMEPtoEvent.size()-1;


      vector<pair<int, int> > usePattern;  // 主要是將 screen 關閉太久後，就當做前後沒有關係
      getUserPattern(&usePattern, eventVec);

      // 整理後給 DMEventPoint
      putOnDMEventPoint(&usePattern, eventVec);
    }

    bool getUserPattern(vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 看螢幕"暗著"的時間間隔 // mason
      cout << "    ========= Interval Time > " << intervalTime.hour << " hour ==========" <<endl;
      cout << "              start                     end     interval" <<endl;
      Event *screenChgOnShut, *screenChgOffShut;
      pair<int, int> onePattern = make_pair(0,0);
      int pI;
      // 先找第一次變亮的時候
      for (pI = 0; pI<eventVec->size(); pI++) {
        if (eventVec->at(pI).isThisScreenOn) {
          screenChgOnShut = &(eventVec->at(pI));
          onePattern.first = pI;
          break;
        }
      }
      // PI 有起頭之後就用 while 跑
      while (pI<eventVec->size()) {
        // 再找第一次變暗的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (!eventVec->at(pI).isThisScreenOn) {
            screenChgOffShut = &(eventVec->at(pI));
            onePattern.second = pI;
            break;
          }
        }
        // 這裡的 screenChgOnShut screenChgOffShut 之間是 螢幕亮的期間

        // 新的一次 找第一次變亮的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (eventVec->at(pI).isThisScreenOn) {
            screenChgOnShut = &(eventVec->at(pI));
            break;
          }
        }

        // 先檢察是不是還在 pattern 範圍內
        if (!(pI<eventVec->size())) {
          break;
        }
        // 有找到的話 檢查中間間隔是不是夠久
        // 比 intervalTime 還長的話就代表有 並紀錄
        else if (*(screenChgOnShut->thisDate) - *(screenChgOffShut->thisDate) > intervalTime) {
          usePattern->push_back(onePattern);
          onePattern.first = pI;
          /*screenChgOffShut->thisDate->output();
          cout << "\t";
          screenChgOnShut->thisDate->output();
          cout << "\t";
          (*(screenChgOnShut->thisDate) - *(screenChgOffShut->thisDate)).output();
          cout << endl;*/
        }
      }
      return true;
    }

    /** 如果有遇到一次發現多的 app 執行的話
     * 就用 "oom_score" 來判斷先後
     * 數字大的前面
     * 最後都放到 DMEventPoint 之中
     */
    void putOnDMEventPoint(vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 用 usePattern 來取出
      for (int i=0; i<usePattern->size(); i++) {
        pair<int, int> useInterval = usePattern->at(i);
        // EP = EventPoint
        for (int EP=useInterval.first; EP<=useInterval.second; EP++) {
          /** 順序是先
           * 1. check screen, if turn on, record it
           * 2. record app
           * 3. check screen, if turn off, record it
           */
          Event* oneshut = &(eventVec->at(EP));
          // ----- 1. check screen, if turn on, record it
          if (!oneshut->isThisScreenOn && oneshut->isNextScreenOn) {
            DMEventPoint.push_back(EPscreen_turn_on);
          }

          // ----- 2. record app
          vector<Event::Case> *caseVec = &(oneshut->caseVec);
          // 有東西才記錄
          if (caseVec->size()!=0) {
            bool app_record[caseVec->size()];
            for (int j=0; j<caseVec->size(); j++) {
              app_record[j] = false;
            }
            for (int j=0; j<caseVec->size(); j++) {
              int big_oom_score = -1;
              int big_app_num = -1;
              int big_app_name = -1;
              for (int k=0; k<caseVec->size(); k++) {
                if (app_record[k]) {
                  continue;
                }
                if (big_oom_score <= caseVec->at(k).nextApp->oom_score) {
                  big_oom_score = caseVec->at(k).nextApp->oom_score;
                  big_app_num = k;
                  big_app_name = caseVec->at(k).nextApp->namePoint;
                }
              }
              if (big_oom_score<0 || big_app_num<0 || big_app_name<0) {
                cout << "bad" <<endl;
              }
              DMEventPoint.push_back(big_app_name);
              //cout << big_oom_score << " : " << big_app_name;
              //cout << "\t" << caseVec->at(j).nextApp->oom_score << " : " << caseVec->at(j).nextApp->namePoint;
              //cout << endl;
              app_record[big_app_num] = true;
            }
          }
          
          // ----- 3. check screen, if turn off, record it
          if (oneshut->isThisScreenOn && !oneshut->isNextScreenOn) {
            DMEventPoint.push_back(EPscreen_turn_off);
          }
        }
        // 接下來是很久時間都沒打開螢幕
        DMEventPoint.push_back(EPscreen_long_off);
      }
    }
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
  DataMining dataMining;
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
  //netCDFoutput.collection(&collecAllData.allEventVec, &collecAllData.allAppNameVec);
  dataMining.collection(&collecAllData.allEventVec, &collecAllData.allAppNameVec);
  
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
