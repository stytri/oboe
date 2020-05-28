@echo ### %1 ...
@..\oboe -o ./out/%1.log %*
@more .\out\%1.log
@IF EXIST "./ref/%1.log" (
	diff -q ./ref/%1.log ./out/%1.log
)
