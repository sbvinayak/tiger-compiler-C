all: main
# parsetest parsetest_display

#FINAL PHASE SUBMISSION WITH OPTIMIZATION
# Link to generate the main executable: tiger
main: main.o semant.o types.o table.o y.tab.o lex.yy.o errormsg.o util.o symbol.o absyn.o tree.o temp.o frame.o translate.o printtree.o canon.o assem.o codegen.o flowgraph.o graph.o prabsyn.o constants.o retreplace.o retfold.o retscan.o
	cc -o tiger -g main.o semant.o types.o table.o symbol.o absyn.o y.tab.o lex.yy.o errormsg.o util.o tree.o temp.o frame.o translate.o printtree.o canon.o assem.o codegen.o flowgraph.o graph.o prabsyn.o constants.o retreplace.o retfold.o retscan.o

main.o: main.c
	cc -g -c main.c

codegen.o: mipscodegen.c codegen.h
	cc -g -o codegen.o -c mipscodegen.c

canon.o: canon.c canon.h
	cc -g -c canon.c

assem.o: assem.c assem.h
	cc -g -c assem.c

flowgraph.o: flowgraph.c flowgraph.h
	cc -g -c flowgraph.c

graph.o: graph.c graph.h
	cc -g -c graph.c
	
prabsyn.o: prabsyn.c prabsyn.h
	cc -g -c prabsyn.c

retreplace.o: retreplace.c retrace.h
	cc -g -c retreplace.c

retfold.o: retfold.c retrace.h
	cc -g -c retfold.c

retscan.o: retscan.c retrace.h
	cc -g -c retscan.c
	
constants.o: constants.c constants.h
	cc -g -c constants.c
	 
# Added for Assignment 6 END

parsetest_display: semant.o types.o table.o y.tab.o lex.yy.o errormsg.o util.o symbol.o absyn.o parsetest_display.o tree.o temp.o frame.o translate.o printtree.o
	cc -o parsetest_display -g semant.o types.o table.o symbol.o absyn.o y.tab.o lex.yy.o errormsg.o util.o parsetest_display.o tree.o temp.o frame.o translate.o printtree.o

parsetest: semant.o types.o table.o y.tab.o lex.yy.o errormsg.o util.o symbol.o absyn.o parsetest.o tree.o temp.o frame.o translate.o printtree.o
	cc -o parsetest -g semant.o types.o table.o symbol.o absyn.o y.tab.o lex.yy.o errormsg.o util.o parsetest.o tree.o temp.o frame.o translate.o printtree.o

parsetest.o: parsetest.c errormsg.h util.h absyn.h
	cc -g -c parsetest.c

parsetest_display.o: parsetest.c errormsg.h util.h absyn.h
	cc -DDISPLAY -g -c parsetest.c -o parsetest_display.o

# Added for Assignment 5 START
translate_test: translate.o
	cc -o translate_test -g translate.o frame.o util.o temp.o table.o symbol.o tree.o

frame_test: frame.o
	cc -o frame_test -g frame.o util.o temp.o table.o symbol.o

translate.o: translate.c translate.h frame.h translate.h mipsframe.h tree.h
	cc -g -c translate.c

frame.o: mipsframe.c mipsframe.h frame.h
	cc -o frame.o -g -c mipsframe.c

printtree.o: printtree.c printtree.h
	cc -g -c printtree.c

tree.o: tree.c tree.h
	cc -g -c tree.c

temp.o: temp.c temp.h
	cc -g -c temp.c
# Added for Assignment 5 END

table.o: table.c
	cc -g -c table.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.grm
	yacc -dv tiger.grm

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
	cc -g -c lex.yy.c

absyn.o: absyn.c absyn.h
	cc -g -c absyn.c

types.o: types.c types.h
	cc -g -c types.c

semant.o: semant.c semant.h
	cc -g -c semant.c
#lex.yy.c: tiger.lex
#	lex tiger.lex

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f tiger parsetest parsetest_display nonPropAsynTree.s PropAsynTree.s a.out *.o y.tab.c y.tab.h y.tab.o
