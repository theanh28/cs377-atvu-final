_DEPS = bank.h ledger.h ledger_queue.h
_OBJ = bank.o ledger.o ledger_queue.o
_MOBJ = main.o
_TOBJ = test.o

GENBIN = gen_ledger
APPBIN = bank_app
TESTBIN = bank_test

IDIR = include
CC = g++
CFLAGS = -I$(IDIR) -Wall -Wextra -g -pthread
ODIR = obj
SDIR = src
LDIR = lib
TDIR = test
LIBS = -lm
XXLIBS = $(LIBS) -lstdc++ -lgtest -lgtest_main -lpthread
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
MOBJ = $(patsubst %,$(ODIR)/%,$(_MOBJ))
TOBJ = $(patsubst %,$(ODIR)/%,$(_TOBJ)) 

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(TDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(APPBIN) $(GENBIN) submission

$(APPBIN): $(OBJ) $(MOBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(GENBIN): src/generate_ledger.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# $(TESTBIN): $(TOBJ) $(OBJ)
# 	$(CC) -o $@ $^ $(CFLAGS) $(XXLIBS)

submission:
	find . -name "*~" -exec rm -rf {} \;
	zip -r submission src lib include


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	rm -f $(APPBIN) $(TESTBIN)
	rm -f submission.zip
