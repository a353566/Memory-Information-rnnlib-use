TARGETS = read
RNNLIB = /home/mason/rnnlib/rnn/include/
libnetcdf_c++.so = /home/mason/rnnlib/rnn/lib/libnetcdf_c++.so
# 設定變數 之後用 $(xxx)即可以使用

.PHONY: clean
# 表示 "clean" 不是一個真正的檔案目標，只是一個標記

all: clean $(TARGETS)
	./$(TARGETS) ../test3/
# ./xxx 是順便執行

clean:
	rm -f $(TARGETS)

$(TARGETS): $(TARGETS).cpp
	g++ -o $@ -I$(RNNLIB) $^ $(libnetcdf_c++.so)
# -I/aaa/bbb 以"/aaa/bbb"當lib的目錄

%: %.cpp
	g++ -o $@ $^
# -o xxx 表示以 xxx 當輸出檔名
# out: in1.c in2.c
# $@ 表示"目標檔名" (out)
# $< 表示"需求檔的第一個" (in1.c)
# $^ 表示"所有需求檔" (in1.c in2.c)
