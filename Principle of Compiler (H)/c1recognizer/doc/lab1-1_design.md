# Lab1-1 Lexer设计方案与分析
## 实验环境
本次实验通过编写文法描述文件（.g4文件），使用ANTLR v4自动生成词法分析器，实现对C1语言的词法分析。
## 实验设计
###
### 1.Token的总体概况
* 由于C1语言仅为C语言的一个子集，故所需实现的token较少，且在原实验文件包中也已预先给出，故无需做其他调整。
* 大部分token可直接由对应符号加' '实现。
* 特殊的几个多格式的token由正规式写出（在下一点中写出）。
* token中不包括需要执行省略操作的空白字符和注释。
### 2.几种特殊的Token
* Identifier：
    * C1语言中的自定义变量名应由字母或下划线开头，并由字母、下划线和数字组成。
    * 对该Token的设计直接由正规式链接即可，除了第一个字符的限定外，直接用()*的形式即可完成。
* IntConst：
    * 整形数字在本次实验中按进制主要分为3类：十六进制、十进制、八进制。
    * 三种进制对应用三个正规式表示即可，并用|链接，其中十六进制开头必须有0X或0x，八进制开头必须有0。
    * 对于数字前的正负号，不和数字一起读入intconst的token，而是存入原来对应的符号token。
    * 特别注意的是，由于除了八进制外不会有整形数字以0开头，且八进制中0后仍应有数字，为了考虑八进制数的正确性，故如果0后没有数字即不成立。故单个数字0需要额外考虑，因此设计时单独加入了一个数字0。
* FloatConst：
    * 根据C语言中的定义：实数应由一个带小数点的十进制数和e的次方数联和表示，后者可省略。
    * 由定义可知必须要有'.'才为一实数，而后面的e/E与加减号和数字均可省略，故前面一部分类似两个整数的链接，后面指数部分最外加一个()?即可。
### 3.省略部分的定义
* 省略部分主要分为空白类字符和注释。
* 空白字符原文档中已给出，即由空格、tab、换行组成，其中换行再不同的操作系统中分别由"\n"或"\r\n"组成，故均需要考虑在内。
* 注释部分为本次实验调试的难点，这里只介绍设计思路，具体设计过程中的问题详见lab1-1问题讨论文档。
    * 设计时主要将注释分为3种：单行注释、双行注释、多行注释。其具体格式可见原实验说明文档。
    * 注释部分最重要的是".*?"的使用，即匹配任意最少的字符直到实现本次匹配，在几种不同的注释中均需要用这个来补齐注释内的文字。
    *  对于不同的注释，只需要按照原格式将对应的注释字符和注释内字符写成正规式即可。
## 实验分析
* 结合本次实验的测试文件来分析该Lexer。
* 对于实验包中已事先提供的四个测试文件，简单测试了所有基本的token，但没考虑许多别的格式的数字和注释等，故在通过测试后添加了自己的测试文件在test_cases/lexer文件夹下。
* 对于自己的测试文件，主要添加了对数字类型和对注释的测试，正确的测试分别为pt_int_float_test.c1和pt_comment.c1，前者测试了所有整形和浮点型数字是否能被完整记录入token，结果如下（部分节选举例）：  
```c
[@25,57:66='0x112abdEf',<IntConst>,5:8]
[@30,78:82='32434',<IntConst>,6:9]
[@34,93:98='012327',<IntConst>,7:8]
[@42,120:129='24321.3232',<FloatConst>,9:8]
[@47,141:148='5.323e+4',<FloatConst>,10:9]
[@51,159:166='7.43E-54',<FloatConst>,11:8]
[@55,177:184='5.20e520',<FloatConst>,12:8]
```
即整形与浮点型数字的识别均无问题。
* 对于注释的测试文件，测试结果节选如下：
```c
[@0,68:69='fl',<Identifier>,3:0]
[@1,94:96='oat',<Identifier>,3:26]
[@2,98:98='a',<Identifier>,3:30]
[@3,99:99='[',<'['>,3:31]
[@4,100:100='5',<IntConst>,3:32]
[@5,101:101=']',<']'>,3:33]
[@6,102:102=';',<';'>,3:34]
[@7,105:107='int',<'int'>,4:0]
[@8,109:112='main',<Identifier>,4:4]
[@9,113:113='(',<'('>,4:8]
[@10,114:114=')',<')'>,4:9]
[@11,116:116='{',<'{'>,5:0]
[@12,123:125='int',<'int'>,6:4]
[@13,127:127='i',<Identifier>,6:8]
[@14,129:129='=',<'='>,6:10]
[@15,131:131='1',<IntConst>,6:12]
[@16,132:132=';',<';'>,6:13]
[@17,183:189='comment',<Identifier>,10:4]
[@18,191:191='*',<'*'>,10:12]
[@19,192:192='/',<'/'>,10:13]
[@20,198:202='while',<'while'>,11:4]
[@21,204:204='(',<'('>,11:10]
[@22,205:205='i',<Identifier>,11:11]
[@23,207:207='>',<'>'>,11:13]
[@24,209:209='0',<IntConst>,11:15]
[@25,210:210=')',<')'>,11:16]
[@26,216:216='{',<'{'>,12:4]
[@27,226:227='if',<'if'>,13:8]
[@28,229:229='(',<'('>,13:11]
[@29,230:230='i',<Identifier>,13:12]
[@30,232:232='<',<'<'>,13:14]
[@31,234:234='5',<IntConst>,13:16]
[@32,235:235=')',<')'>,13:17]
[@33,245:245='a',<Identifier>,14:8]
[@34,246:246='[',<'['>,14:9]
[@35,247:247='i',<Identifier>,14:10]
[@36,249:249='-',<'-'>,14:12]
[@37,251:251='1',<IntConst>,14:14]
[@38,252:252=']',<']'>,14:15]
[@39,254:254='=',<'='>,14:17]
[@40,256:256='i',<Identifier>,14:19]
[@41,257:257=';',<';'>,14:20]
[@42,267:270='else',<'else'>,15:8]
[@43,272:272='i',<Identifier>,15:13]
[@44,274:274='=',<'='>,15:15]
[@45,276:276='0',<IntConst>,15:17]
[@46,277:277=';',<';'>,15:18]
[@47,342:342='i',<Identifier>,18:8]
[@48,343:343='+',<'+'>,18:9]
[@49,344:344='+',<'+'>,18:10]
[@50,345:345=';',<';'>,18:11]
[@51,351:351='}',<'}'>,19:4]
[@52,408:408='a',<Identifier>,24:4]
[@53,409:409='[',<'['>,24:5]
[@54,410:410='0',<IntConst>,24:6]
[@55,411:411=']',<']'>,24:7]
[@56,413:413='=',<'='>,24:9]
[@57,415:415='0',<IntConst>,24:11]
[@58,416:416=';',<';'>,24:12]
[@59,418:418='}',<'}'>,25:0]
[@60,419:418='<EOF>',<EOF>,25:1]
```
可见已完成对应注释的省略工作。
* 对于错误的示例，即命名为ft_的文件，其中三个文件共完成了对应的三种考虑：
    * 1.由于C1语言中对各种符号识别较为有限，像"、\、#这种本来C中较为常用的符号，在C1语言中没有识别，因此当文件中出现此类符号时，运行结果会直接报错，举例如下：
    * 2.对于数字的错误，当任意数字前有多余的0时(八进制数除外)，即会出现一个数字被拆分为2个或多个数字，如"0925"被拆分为"0"和"925"读入。第二类错误为数字未加0x但出现了字母或加了0x但数字中出现了除a-f外的字母，此时会将该数字拆分成两个token，前者为intconst，后者为identifier。例如对"2453adf34a"，即拆分为"2453"和"adf34a"。如上情况虽然也能正常识别，但仍然为一类识别错误，应在后面的语义分析阶段报错指出。
    * 3.第三类错误出现在注释中。当注释拆分原来的token时，原token会被拆分为两个，且不会直接被合并成一个原本应有的token，如"el/*abc*/se"被读出时会显示为el和se的identifier类型，即出现此类错误。另一个错误是当出现多行注释嵌套的情况时，在运行结果中注释的结束点仅为第一个"*/"，而后的内容会被继续读入做token(此错误可能为编程类错误，故暂时没有给出解决方案)。
## 实验问题与解决方案
* 大部分设计中的简单问题已在如上的实验设计部分中指出，此处列出的均为处理时间较长或仍未处理成功的问题。
* Q1：在实现注释时不同注释相互冲突，主要表现在双行注释"//...\\n...\n"与单行注释的冲突，两种注释在考虑初期出现了单行注释的//被误认为是下一个双行注释的//，并一下导致到下一个\前的所有内容均被视为注释的情况。  
A：对于本问题最初的处理方法为在双行注释的.*?前加上~'//'来处理该问题，即在处理双行注释时应考虑只接收一个//，避免被单行注释误导。在完成上述处理后发现原问题仍存在，该考虑并未完成原问题的处理。故调换另一方案为：合并该单行和双行注释的词条，他们使用同一个开头的//，考虑到匹配时采取最大匹配的原则，故应可以处理该问题，最后测试结果通过。
* Q2：对于前面提到的错误示例中存在的几个错误问题，如对多重注释的错误读取、插入注释对关键词的分割，均是本次实验未实现的部分，可能应考虑在编写程序阶段就避免此类错误还是在后续的语义分析阶段再进行处理？