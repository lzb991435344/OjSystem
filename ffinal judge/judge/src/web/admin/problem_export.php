<?php require_once("admin-header.php");
if (!(isset($_SESSION[$OJ_NAME.'_'.'administrator']))){
	echo "<a href='../loginpage.php'>Please Login First!</a>";
	exit(1);
}
?>

<div class="container">
<form action='problem_export_xml.php' method=post>
 <h1> Atention: </h1>
         <h3>install fps file .xml, problem_ids like [1000,1020]</br> </h3>
	<h3>Export Problem:</h3><br />
	from pid:<input type=text size=10 name="start" value=1000>
	to pid:<input type=text size=10 name="end" value=1000><br />
	<input type='hidden' name='do' value='do'>
       <input type=submit value='Download'>
   <?php require_once("../include/set_post_key.php");?>
</form>
</div>
