<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">
    <link rel="icon" href="../../favicon.ico">

    <title><?php echo $OJ_NAME?></title>  
    <?php include("template/$OJ_TEMPLATE/css.php");?>	    


  </head>

  <body>

    <div class="container">
    <?php include("template/$OJ_TEMPLATE/nav.php");?>	    
      <!-- Main component for a primary marketing message or call to action -->
      <div class="jumbotron">
<hr>
<center>
  <font size="+3"><?php echo $OJ_NAME?> Online Judge FAQ</font>
</center>
<hr>

<p>  编译器版本:<br>
  <font color=blue>gcc version 4.8.4 (Ubuntu 4.8.4-2ubuntu1~14.04.3)</font><br>
  <font color=blue>glibc 2.19</font><br>
 <font color=blue>openjdk 1.7.0_151</font><br>
</font></p>
<hr>

<p>This is the Answer About Problem 1000<br></p>
<p> C++:<br>
</p>
<pre><font color="blue">
#include &lt;iostream&gt;
using namespace std;
int main(){
    int a,b;
    while(cin >> a >> b)
        cout << a+b << endl;
    return 0;
}
</font></pre>

<p>C:<br> </p>
<pre><font color="blue">
#include &lt;stdio.h&gt;
int main(){
    int a,b;
    while(scanf("%d %d",&amp;a, &amp;b) != EOF)
        printf("%d\n",a+b);
    return 0;
}

</font></pre>
<br><br> 

<p>Java:<br> </p>
<pre><font color="blue">
import java.util.*;
public class Main{
	public static void main(String args[]){
		Scanner cin = new Scanner(System.in);
		int a, b;
		while (cin.hasNext()){
			a = cin.nextInt(); b = cin.nextInt();
			System.out.println(a + b);
		}
	}
}</font></pre>


<hr>
<p><font color=green>Q</font>:系统返回信息都是什么意思?<br> </p>
<p><font color=blue>Pending</font> : 系统忙，你的答案在排队等待. </p>
<p><font color=blue>Pending Rejudge</font>: 因为数据更新或其他原因，系统将重新判你的答案.</p>
<p><font color=blue>Compiling</font> : 正在编译.<br>
</p>
<p><font color="blue">Running &amp; Judging</font>: 正在运行和判断.<br>
</p>
<p><font color=blue>Accepted</font> : 程序通过!<br>
  <br>
  <font color=blue>Presentation Error</font> : 答案基本正确，但是格式不对。<br>
  <br>
  <font color=blue>Wrong Answer</font> : 答案不对，仅仅通过样例数据的测试并不一定是正确答案，一定还有你没想到的地方.<br>
  <br>
  <font color=blue>Time Limit Exceeded</font> : 运行超出时间限制，检查下是否有死循环，或者应该有更快的计算方法。<br>
  <br>
  <font color=blue>Memory Limit Exceeded</font> : 超出内存限制，数据可能需要压缩，检查内存是否有泄露。<br>
  <br>
  <font color=blue>Output Limit Exceeded</font>: 输出超过限制，你的输出比正确答案长了两倍.<br>
  <br>
  <font color=blue>Runtime Error</font> : 运行时错误，非法的内存访问，数组越界，指针漂移，调用禁用的系统函数。请点击后获得详细输出。<br>
</p>
<p>  <font color=blue>Compile Error</font> : 编译错误，请点击后获得编译器的详细输出。<br>
  <br>
</p>

<!--<center>
  <table width=100% border=0>
    <tr>
      <td align=right width=65%>
      <a href = "index.php"><font color=red><?php echo $OJ_NAME?></font></a> 
      <a href = "https://github.com/zhblue/hustoj"><font color=red>17.12.01</font></a></td>
    </tr>
  </table>
</center> --> 
      </div>

    </div> 


   <?php include("template/$OJ_TEMPLATE/js.php");?>    

  </body>
</html>
