# Lexer程序设计思路及思考
## 设计思路
1.整体框架：整体程序采用了状态机的设计框架，通过ppt上简单的九个状态来实现整个转换图。   
2.整体封装：本次实验中封装了一个trans_diag的类来实现对符号词法的分析，在trans_diag类中自带了对符号的识别、定位和打印等。   
3.核心思路：主要实现了getrelop函数，其实现了对读入数据完成下一个符号的识别。加入一个变量n来确定当前识别出的符号在原字符流中的准确位置，每次将上一个符号识别出的位置作为下一次识别的开始位置，循环下去直到完成整个文档的识别。   
4.完成了trans_diag类的封装后，设计了test.cpp的测试文件，实现了从文件中边读取字符流边识别出符号的测试目的，循环终止条件为文件读完，循环进行符号的识别和符号识别结果的打印。   
## 设计问题
在实际的设计中，考虑过以下问题：   
1.在实际编写中，由于从文件中读入的字符流较容易进行顺序阅读，所以在出现4和8状态，即回退一个位置时，存在较大问题。因此为了避免修改整体框架，加入了一个ret变量，用以判断是否发生了回退情况，当发生了回退时将ret置1，同时保存上一个读入字符的值。因此在每次读入字符前，加入判断语句对ret进行判断，若ret为1，不重新读入字符，直接返回之前复制的上一个字符即可。
2、对于开始时出现的其他标识符情况，均未进行考虑，直接等待程序找到正确地标识符。ppt中提到的出错处理的意义仍存疑。