<?php require_once("admin-header.php");?>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<title>Add a contest</title>

<?php
	require_once("../include/db_info.inc.php");
	require_once("../lang/$OJ_LANG.php");
	require_once("../include/const.inc.php");

$description="";
 if (isset($_POST['startdate']))
{
	
	require_once("../include/check_post_key.php");
	
	$starttime=$_POST['startdate']." ".intval($_POST['shour']).":".intval($_POST['sminute']).":00";
	$endtime=$_POST['enddate']." ".intval($_POST['ehour']).":".intval($_POST['eminute']).":00";
	//	echo $starttime;
	//	echo $endtime;

        $title=$_POST['title'];
        $private=$_POST['private'];
        $password=$_POST['password'];
        $description=$_POST['description'];
        if (get_magic_quotes_gpc ()){
                $title = stripslashes ($title);
                $private = stripslashes ($private);
                $password = stripslashes ($password);
                $description = stripslashes ($description);
        }

    $lang=$_POST['lang'];
    $langmask=0;
    foreach($lang as $t){
			$langmask+=1<<$t;
	} 
$langmask=((1<<count($language_ext))-1)&(~$langmask);
	//echo $langmask;	
	
        $sql="insert INTO `contest`(`title`,`start_time`,`end_time`,`private`,`langmask`,`description`,`password`)
                VALUES(?,?,?,?,?,?,?)";
	echo $sql.$title.$starttime.$endtime.$private.$langmask.$description.$password;
	$cid=pdo_query($sql,$title,$starttime,$endtime,$private,$langmask,$description,$password) ;
	echo "Add Contest ".$cid;
	$sql="DELETE FROM `contest_problem` WHERE `contest_id`=$cid";
	$plist=trim($_POST['cproblem']);
	$pieces = explode(",",$plist );
	if (count($pieces)>0 && intval($pieces[0])>0){
		$sql_1="INSERT INTO `contest_problem`(`contest_id`,`problem_id`,`num`) 
			VALUES (?,?,?)";
		$plist="";
		for ($i=0;$i<count($pieces);$i++){
			
			if($plist)$plist.=",";
			$plist.=$pieces[$i];
			pdo_query($sql_1,$cid,$pieces[$i],$i);
		}
		//echo $sql_1;
		
		$sql="update `problem` set defunct='N' where `problem_id` in ($plist)";
		pdo_query($sql) ;
	}
	$sql="DELETE FROM `privilege` WHERE `rightstr`=?";
	pdo_query($sql,"c$cid");
	$sql="insert into `privilege` (`user_id`,`rightstr`)  values(?,?)";
	pdo_query($sql,$_SESSION[$OJ_NAME.'_'.'user_id'],"m$cid");
	$_SESSION[$OJ_NAME.'_'."m$cid"]=true;
	$pieces = explode("\n", trim($_POST['ulist']));
	if (count($pieces)>0 && strlen($pieces[0])>0){
		$sql_1="INSERT INTO `privilege`(`user_id`,`rightstr`) 
			VALUES (?,?)";
		for ($i=0;$i<count($pieces);$i++){
			
			pdo_query($sql_1,trim($pieces[$i]),"c$cid") ;
		}
	}
	echo "<script>window.location.href=\"contest_list.php\";</script>";
}
else{
	
   if(isset($_GET['cid'])){
		   $cid=intval($_GET['cid']);
		   $sql="select * from contest WHERE `contest_id`=?";
		   $result=pdo_query($sql,$cid);
		    $row=$result[0];
		   $title=$row['title'];
		   
			$plist="";
			$sql="SELECT `problem_id` FROM `contest_problem` WHERE `contest_id`=? ORDER BY `num`";
			$result=pdo_query($sql,$cid) ;
			foreach($result as $row){
				if ($plist) $plist=$plist.',';
				$plist=$plist.$row[0];
			}
			
   }
else if(isset($_POST['problem2contest'])){
	   $plist="";
	   //echo $_POST['pid'];
	   sort($_POST['pid']);
	   foreach($_POST['pid'] as $i){		    
			if ($plist) 
				$plist.=','.intval($i);
			else
				$plist=$i;
	   }
}else if(isset($_GET['spid'])){
	require_once("../include/check_get_key.php");
		   $spid=intval($_GET['spid']);
		 
			$plist="";
			$sql="SELECT `problem_id` FROM `problem` WHERE `problem_id`>=? ";
			$result=pdo_query($sql,$spid) ;
			foreach ($result as $row){
				if ($plist) $plist.=',';
				$plist.=$row[0];
			}
			
}  
  include_once("kindeditor.php") ;
?>
	
<div class="container">
	<form method=POST >
	<p align=left><?php echo $MSG_TITLE?><input class=input-xxlarge  type=text name=title size=71 value="<?php echo isset($title)?$title:""?>"></p>
	<p align=left><?php echo $MSG_Start?>:
	<input  class=input-large type=date name='startdate' value='<?php echo date('Y').'-'. date('m').'-'.date('d')?>' size=4 >
	Hour:<input class=input-mini    type=text name=shour size=2 value=<?php echo date('H')?>>&nbsp;
	Minute:<input class=input-mini    type=text name=sminute value=00 size=2 ></p>
	<p align=left><?php echo $MSG_End?>
	<input  class=input-large type=date name='enddate' value='<?php echo date('Y').'-'. date('m').'-'.date('d')?>' size=4 >
	Hour:<input class=input-mini  type=text name=ehour size=2 value=<?php echo (date('H')+4)%24?>>&nbsp;
	Minute:<input class=input-mini  type=text name=eminute value=00 size=2 ></p>
	<?php echo $MSG_Public?>:<select name=private><option value=0><?php echo $MSG_Public?></option>
				    <option value=1><?php echo $MSG_Private?></option>
               </select>
	<?php echo $MSG_PASSWORD?>:<input type=text name=password value="">
	<?php require_once("../include/set_post_key.php");?>
	<br><?php echo $MSG_PROBLEM_ID?>:<input class=input-xxlarge placeholder="Example:1000,1001,1002" type=text size=60 name=cproblem value="<?php echo isset($plist)?$plist:""?>">
	<br>
	<p align=left><?php echo $MSG_Description?><br><textarea class=kindeditor rows=13 name=description cols=80></textarea>
	<?php echo $MSG_LANG?>:<select name="lang[]" multiple="multiple"    style="height:220px">
	<?php
$lang_count=count($language_ext);

 $langmask=$OJ_LANGMASK;

 for($i=0;$i<$lang_count;$i++){
                 echo "<option value=$i selected>
                        ".$language_name[$i]."
                 </option>";
  }

?>


        </select>


	<?php echo $MSG_USER?><textarea name="ulist" rows="12" cols="20" placeholder=""></textarea>
<input type=submit value="<?php echo $MSG_ADD?>" name=submit> </br></br>
<b>可以将学生学号从Excel整列复制过来，然后要求他们用学号做UserID注册,就能进入Private的比赛作为作业和测验。 </b>
	
	</form>
</div>
<?php }
require_once("../oj-footer.php");

?>

