! 1 
! 1 # 1 "test.c"
! 1 const int num = 5;
!BCC_EOS
fail! test.c:1.9: error: need ';'
.data
export	_num
_num:
.word	5
!BCC_EOS
! 2 
! 3 int add(int a, int b) {
fail! test.c:3.11: error: need ')'
.text
export	_add
_add:
fail! test.c:3.14: error: a not in argument list
fail! test.c:3.18: error: need variable name
fail! test.c:3.20: error:  not in argument list
!BCC_EOS
fail! test.c:3.20: error: need ';'
fail! test.c:3.20: error: need '{'
push	bp
mov	bp,sp
push	di
push	si
fail! test.c:3.21: error: b undeclared
!BCC_EOS
fail! test.c:3.21: error: need ';'
fail! test.c:3.21: error: bad expression
!BCC_EOS
fail! test.c:3.23: error: need ';'
! 4 	return a + b;
! Debug: add int = [b+0] to int = [a+0] (used reg = )
mov	ax,[_a]
add	ax,[_b]
! Debug: cast int = const 0 to int = ax+0 (used reg = )
pop	si
pop	di
pop	bp
ret
!BCC_EOS
! 5 }
! 6 
! 7 int main() {
fail! test.c:7.3: error: bad expression
!BCC_EOS
fail! test.c:7.8: error: need ';'
! Debug: func () int = main+0 (used reg = )
call	_main
!BCC_EOS
fail! test.c:7.12: error: need ';'
! 8 	add(5, num);
! Debug: list int = [num+0] (used reg = )
push	[_num]
! Debug: list int = const 5 (used reg = )
mov	ax,*5
push	ax
! Debug: func () int = add+0 (used reg = )
call	_add
add	sp,*4
!BCC_EOS
! 9 }
! 10 
pop	si
pop	di
pop	bp
ret
fail! test.c:eof: error: need '}'
.data
.bss
.comm	_const,2
.comm	_b,2
.comm	_a,2
.fail	15 errors detected

! 15 errors detected
