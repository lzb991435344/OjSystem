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

      <div class="jumbotron">
<hr>
<center>
  <font size="+3"><?php echo $OJ_NAME?>  FAQ</font>
</center>
<hr>

<p>  Our compiler software version:<br>
  <font color=blue>gcc (Ubuntu/Linaro 4.4.4-14ubuntu5) 4.4.5</font><br>
  <font color=blue>glibc 2.3.6</font><br>

java version "1.7"<br>
</font></p>
<hr>

Here is a sample solution for problem 1000 using C++:<br>
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
Here is a sample solution for problem 1000 using C:<br>
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

Here is a sample solution for problem 1000 using Java:<br>
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

      </div>

    </div> <!-- /container -->


    <!-- Bootstrap core JavaScript
    ================================================== -->
    <!-- Placed at the end of the document so the pages load faster -->
    <?php include("template/$OJ_TEMPLATE/js.php");?>	    
 
</script>
  </body>
</html>
