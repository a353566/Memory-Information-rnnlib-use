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
    
#ifdef NEW
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
                cout << "(error) CollectionFile::openFileAndRead() appTotalPss: " << appTotalPss <<endl;
                appDataGood = false;
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
#else
    // 開啟檔案，並開始讀檔
    bool openFileAndRead() {
      // 開檔部分
      FILE *file;
      file = fopen(fileName.c_str(),"r");
      if(file == NULL) {
        cout << "open \"" << fileName << "\" File fail" << endl ;
        return false;
      }
      
      char getLine[4096];
      int line=0;
      // 前三行跳過
      while (line < 3) {
        if (fgets(getLine, 4096, file) != NULL) {
          line++;
        } else {
          fclose(file);
          return false;
        }
      }
      
      // 第一筆資料之前 和 "----------" 一樣才繼續
      if (fgets(getLine, 4096, file) != NULL) {
        line++;
        if (strstr(getLine,"----------") == NULL) {
          fclose(file);
          return false;
        }
      } else {
        fclose(file);
        return false;
      }
      
      // ========== 第一筆資料 ==========
      Point firstOneShot;
      
      // 讀取時間 2017-12-16_07:36:39
      if (fgets(getLine, 4096, file) != NULL) {
        line++;
        // 有問題就 false
        if (!firstOneShot.getTime(string(getLine))) {
          fclose(file);
          return false;
        }
      } else {
        fclose(file);
        return false;
      }
      
      // 讀取 app 數量 Big||procNum:13
      if (fgets(getLine, 4096, file) != NULL) {
        line++;
        // 取得 APP 數量
        string tempStr(strstr(getLine,"procNum:"));
        
        // 有問題就 false
        if (!StringToNumber(tempStr.substr(8,tempStr.size() - 9), &firstOneShot.appNum)) {
          fclose(file);
          return false;
        }
      } else {
        fclose(file);
        return false;
      }
      
      //com.google.android.gms  ~~  |3582|40078|86|0|0||com.google.android.googlequicksearchbox:interactor                          |3676|5256|70|0|0||com.android.smspush                                                         |4108|2358|68|0|0||com.google.android.gms.persistent                                           |4129|59180|90|0|0||com.ilike.cartoon                                                           |4317|15966|312|0|4||com.ilike.cartoon:pushservice                                               |4503|8260|485|0|7||com.facebook.katana                                                         |10453|53715|323|0|4||com.facebook.orca                                                           |10824|59213|324|0|4||com.android.vending                                                         |13849|17608|75|0|0||gogolook.callgogolook2                                                      |16465|27391|314|0|4||com.madhead.tos.zh                                                          |16543|518711|179|0|0||com.mason.memoryinfo:remote                                                 |16777|9657|307|0|4||com.facebook.katana:notification                                            |18395|8462|485|0|7||
      // 讀取 app 資料
      firstOneShot.app = new Point::App[firstOneShot.appNum];
      bool haveOom_ajd = false;
      int nameLength = 76+1;    // +1是為了方便
      if (fgets(getLine, 4096, file) != NULL) {
        line++;
        int index = 0;
        for (int getAppNum = 0; getAppNum<firstOneShot.appNum; getAppNum++) {
          Point::App tempApp;
          // 取得名字
          string appName(getLine + index);
          appNameVec.push_back(appName);
          // 判斷 appName 在 appNameVec 中是第幾個
          int appNamePoint = -1;
          for (int i=0; i < appNameVec.size(); i++) {
            if (appNameVec[i] != appName) {
              appNamePoint--;
            } else {
              appNamePoint = -appNamePoint-1;
              break;
            }
          }
          // 有的話將數字放入 tempApp.namePoint
          if (appNamePoint>=0) {
            tempApp.namePoint = appNamePoint;
          } else { // 沒有的話 return false
            fclose(file);
            return false;
          }
          
          // 取得資料，其中都是用 '|' 切割
          string appData(getLine + index + nameLength);
          int start = 0, end;
          // 找 pid，沒找到則回傳 false
          end = appData.find("|", start);
          if (!StringToNumber(appData.substr(start, end - start), &tempApp.pid)) {
            fclose(file);
            return false;
          }
          // 找 TotalPss，沒找到則回傳 false
          start = end + 1;
          end = appData.find("|", start);
          if (!StringToNumber(appData.substr(start, end - start), &tempApp.totalPss)) {
            fclose(file);
            return false;
          }
          // 找 oom_score，沒找到則回傳 false
          start = end + 1;
          end = appData.find("|", start);
          if (!StringToNumber(appData.substr(start, end - start), &tempApp.oom_score)) {
            fclose(file);
            return false;
          }
          // 找 ground，沒找到則回傳 false
          start = end + 1;
          end = appData.find("|", start);
          if (!StringToNumber(appData.substr(start, end - start), &tempApp.ground)) {
            fclose(file);
            return false;
          }
          // attention:oom_adj 大部分到這裡就結束了
          
          // 試著找 oom_adj
          start = end + 1;
          end = appData.find("|", start);
          // 中間還有空間的話，代表有
          if (start < end) {
            haveOom_ajd = true;
            // 這邊有問題的話，還是要回傳 false
            if (!StringToNumber(appData.substr(start, end - start), &tempApp.oom_adj)) {
              fclose(file);
              return false;
            }
            end += 1;
          }
          
          // 算 index
          index += nameLength + end + 1;
          
          // 將 APP 放入 OneShot 中
          firstOneShot.app[getAppNum] = tempApp;
        }
        // -------------------- test --------------------------
        //for (int i=0; i<firstOneShot.appNum; i++)
        //  firstOneShot.app[i].output(appNameVec);
        
      } else {
        fclose(file);
        return false;
      }
      
      // 取得 Screen，有問題 return false
      if (fgets(getLine, 4096, file) != NULL) {
        line++;
        if (strstr(getLine,"Screen:On") != NULL) {
          firstOneShot.screen = true;
        } else if (strstr(getLine,"Screen:Off") != NULL) {
          firstOneShot.screen = false;
        } else {
          fclose(file);
          return false;
        }
      }
      
      while (fgets(getLine, 4096, file) != NULL) {
        line++;
        if (strstr(getLine,"----------") != NULL) {
          break;
        }
      }
      pattern.push_back(firstOneShot);
      // ========== 第一筆資料 結束 ==========
      // ========== 第一筆之後的資料 ==========
      Point lastOneShot = firstOneShot;
      while (true) {
        Point oneShot;
        bool isBigScan;
        // 讀取時間 2017-12-16_07:36:39
        if (fgets(getLine, 4096, file) != NULL) {
          line++;
          if (!oneShot.getTime(string(getLine))) {
            // 有問題就中斷 return true
            fclose(file);
            return true;
          }
        }
        
        // 讀取 什麼類型的搜尋 和 app數量 Big||procNum:13
        if (fgets(getLine, 4096, file) != NULL) {
          line++;
          // 檢查類型 "Big" or "Small"
          if (strstr(getLine,"Big") != NULL) {
            isBigScan = true;
          } else if (strstr(getLine,"Small") != NULL) {
            isBigScan = false;
          } else {
            // 有問題就中斷 return true
            fclose(file);
            return true;
          }
          
          // 取得 APP 數量
          string tempStr(strstr(getLine,"procNum:"));
          int tempN;
          if (StringToNumber(tempStr.substr(8,tempStr.size() - 9), &tempN)) {
            oneShot.appNum = tempN;
          } else {
            // 有問題就中斷 return true
            fclose(file);
            return true;
          }
        } else {
          // 有問題就中斷 return true
          fclose(file);
          return true;
        }
        
        //com.google.android.gms  ~~  |3582|40078|86|0|0||com.google.android.googlequicksearchbox:interactor                          |3676|5256|70|0|0||com.android.smspush                                                         |4108|2358|68|0|0||com.google.android.gms.persistent                                           |4129|59180|90|0|0||com.ilike.cartoon                                                           |4317|15966|312|0|4||com.ilike.cartoon:pushservice                                               |4503|8260|485|0|7||com.facebook.katana                                                         |10453|53715|323|0|4||com.facebook.orca                                                           |10824|59213|324|0|4||com.android.vending                                                         |13849|17608|75|0|0||gogolook.callgogolook2                                                      |16465|27391|314|0|4||com.madhead.tos.zh                                                          |16543|518711|179|0|0||com.mason.memoryinfo:remote                                                 |16777|9657|307|0|4||com.facebook.katana:notification                                            |18395|8462|485|0|7||
        // 讀取 app 資料
        oneShot.app = new Point::App[oneShot.appNum];
        if (fgets(getLine, 4096, file) != NULL) {
          line++;
          if (isBigScan == false) {
            int index = 0;
            for (int getAppNum = 0; getAppNum<oneShot.appNum; getAppNum++) {
              Point::App tempApp;
              
              // 寫入上次的資料 namePoint pid ground
              Point::App *lastApp = lastOneShot.app;
              tempApp.namePoint = lastApp[getAppNum].namePoint;
              tempApp.pid = lastApp[getAppNum].pid;
              tempApp.ground = lastApp[getAppNum].ground;
              
              // 取得資料，其中都是用 '|' 切割
              string appData(getLine + index);
              int start = 0, end;
              // 找 totalPss
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.totalPss)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // 找 oom_score
              start = end + 1;
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.oom_score)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              
              // 找 ground
              start = end + 1;
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.ground)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // attention:oom_adj 大部分到這裡就結束了
              // 有的話則找 oom_adj
              if (haveOom_ajd) {
                start = end + 1;
                end = appData.find("|", start);
                if (!StringToNumber(appData.substr(start, end - start), &tempApp.oom_adj)) {
                  // 有問題就中斷 return true
                  fclose(file);
                  return true;
                }
              }
              
              // 算 index
              index += end + 2;
              
              // 將 APP 放入 OneShot 中
              oneShot.app[getAppNum] = tempApp;
            }
          } else if(isBigScan == true) {
            int index = 0;
            for (int getAppNum = 0; getAppNum<oneShot.appNum; getAppNum++) {
              Point::App tempApp;
              
              // 取得名字
              string appName(getLine + index);
              // 判斷 appName 在 appNameVec 中是第幾個
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
              
              // 取得資料，其中都是用 '|' 切割
              string appData(getLine + index + nameLength);
              int start = 0, end;
              // 找 pid
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.pid)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // 找 TotalPss
              start = end + 1;
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.totalPss)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // 找 oom_score
              start = end + 1;
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start) ,&tempApp.oom_score)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // 找 ground
              start = end + 1;
              end = appData.find("|", start);
              if (!StringToNumber(appData.substr(start, end - start), &tempApp.ground)) {
                // 有問題就中斷 return true
                fclose(file);
                return true;
              }
              // attention:oom_adj 大部分到這裡就結束了
              
              // 有的話找 oom_adj
              if (haveOom_ajd) {
                start = end + 1;
                end = appData.find("|", start);
                if (!StringToNumber(appData.substr(start, end - start), &tempApp.oom_adj)) {
                  // 有問題就中斷 return true
                  fclose(file);
                  return true;
                }
              }
              
              // 算 index
              index += nameLength + end + 2;
              
              // 將 APP 放入 OneShot 中
              oneShot.app[getAppNum] = tempApp;
            }
          } else {
            // 有問題就中斷 return true
            fclose(file);
            return true;
          }
          // -------------------- test --------------------------
          //for (int i=0; i<oneShot.appNum; i++)
          //  oneShot.app[i].output(appNameVec);
          //cout << "============================" <<endl;
        } else {
          // 有問題就中斷 return true
          fclose(file);
          return true;
        }
        
        // 取得 Screen，有問題 return false
        if (fgets(getLine, 4096, file) != NULL) {
          line++;
          if (strstr(getLine,"Screen:On") != NULL) {
            oneShot.screen = true;
          } else if (strstr(getLine,"Screen:Off") != NULL) {
            oneShot.screen = false;
          } else {
            // 有問題就中斷 return true
            fclose(file);
            return true;
          }
        } else {
          // 有問題就中斷 return true
          fclose(file);
          return true;
        }
        
        // attention:G-sensor 非常長 目前最長有看到 32645的
        while (fgets(getLine, 4096, file) != NULL) {
          line++;
          if (strstr(getLine,"----------") != NULL) {
            break;
          }
        }
        lastOneShot = oneShot;
        pattern.push_back(oneShot);
      }
      // ========== 第一筆之後的資料 結束 ==========
      
      // 關檔
      fclose(file);
      return true;
    };
#endif
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