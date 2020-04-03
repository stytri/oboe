@..\oboe -n -g ./out/%1.dot %1
@IF EXIST "./ref/%1.dot" (
	diff -q ./ref/%1.dot ./out/%1.dot
)