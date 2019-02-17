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
	
<center>
<table class="table table-striped" id=statics width=70%>
<caption>
<?php echo $user."--".htmlentities($nick,ENT_QUOTES,"UTF-8")?>
<?php
echo "<a href=mail.php?to_user=$user>$MSG_MAIL</a>";
?>
</caption>
<tr ><td width=15%><?php echo $MSG_Number?><td width=25% align=center><?php echo $Rank?><td width=70% align=center>Solved Problems List</tr>
<tr ><td><?php echo $MSG_SOVLED?><td align=center><a href='status.php?user_id=<?php echo $user?>&jresult=4'><?php echo $AC?></a>
<td rowspan=14 align=center>
<script language='javascript'>
function p(id,c){
	document.write("<a href=problem.php?id="+id+">"+id+" </a>(<a href='status.php?user_id=<?php echo $user?>&problem_id="+id+"'>"+c+"</a>)");

}
<?php $sql="SELECT `problem_id`,count(1) from solution where `user_id`=? and result=4 group by `problem_id` ORDER BY `problem_id` ASC";
if ($result=pdo_query($sql,$user)){ 
    foreach($result as $row)
    echo "p($row[0],$row[1]);";
}

?>
</script>
<div id=submission style="width:600px;height:300px" ></div>
</td>
</tr>
<tr ><td><?php echo $MSG_SUBMIT?><td align=center><a href='status.php?user_id=<?php echo $user?>'><?php echo $Submit?></a></tr>

<tr ><td>School:<td align=center><?php echo $school?></tr>
<tr ><td>Email:<td align=center>***<?php// echo $email?></tr>
</table>
<?php
if(isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><table border=1><tr class=toprow><td>UserID<td>Password<td>IP<td>Time</tr>
<tbody>
<?php
$cnt=0;
foreach($view_userinfo as $row){
	if ($cnt)
		echo "<tr class='oddrow'>";
	else
		echo "<tr class='evenrow'>";
	for($i=0;$i<count($row)/2;$i++){
		echo "<td>";
		echo "\t".$row[$i];
		echo "</td>";
	}
	echo "</tr>";
	$cnt=1-$cnt;
}
?>
</tbody>
</table>
<?php
}
?>
</center>
      </div>

    </div> 

   <?php include("template/$OJ_TEMPLATE/js.php");?> 
<script language="javascript" type="text/javascript" src="include/jquery.flot.js"></script>
<script type="text/javascript">
$(function () {
var d1 = [];
var d2 = [];
<?php
foreach($chart_data_all as $k=>$d){
?>
d1.push([<?php echo $k?>, <?php echo $d?>]);
<?php }?>
<?php
foreach($chart_data_ac as $k=>$d){
?>
d2.push([<?php echo $k?>, <?php echo $d?>]);
<?php }?>

var d3 = [[0, 12], [7, 12], null, [7, 2.5], [12, 2.5]];
$.plot($("#submission"), [
{label:"<?php echo $MSG_SUBMIT?>",data:d1,lines: { show: true }},
{label:"<?php echo $MSG_AC?>",data:d2,bars:{show:true}} ],{
xaxis: {
mode: "time"
},
grid: {
backgroundColor: { colors: ["#ffffff", "#333"] }
} 
});
});
</script> 
 </body>
</html>
