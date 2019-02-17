<?php require_once("admin-header.php");

	if(isset($OJ_LANG)){
		require_once("../lang/$OJ_LANG.php");
	}
	

?>
<html>
<head>
<title><?php echo $MSG_ADMIN?></title>
</head>

<body>
<hr>
<h4>
<ol>
	<li>
		<a class='btn btn-primary' href="../status.php" target="_top" title="<?php echo $MSG_HELP_SEEOJ?>"><b><?php echo $MSG_SEEOJ?></b></a>
<?php if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
	?>
	<li>
	
	<a class='btn btn-primary' href="user_list.php" target="main" title="<?php echo $MSG_HELP_USER_LIST?>"><b><?php echo $MSG_USER.$MSG_LIST?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])||isset($_SESSION[$OJ_NAME.'_'.'problem_editor'])){
?>
	<li>
		<a class='btn btn-primary' href="problem_add_page.php" target="main" title="<?php echo $MSG_HELP_ADD_PROBLEM?>"><b><?php echo $MSG_ADD.$MSG_PROBLEM?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])||isset($_SESSION[$OJ_NAME.'_'.'contest_creator'])||isset($_SESSION[$OJ_NAME.'_'.'problem_editor'])){
?>
	<li>
		<a class='btn btn-primary' href="problem_list.php" target="main" title="<?php echo $MSG_HELP_PROBLEM_LIST?>"><b><?php echo $MSG_PROBLEM.$MSG_LIST?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])||isset($_SESSION[$OJ_NAME.'_'.'contest_creator'])){
?>		
<li>
	<a class='btn btn-primary' href="contest_add.php" target="main"  title="<?php echo $MSG_HELP_ADD_CONTEST?>"><b><?php echo $MSG_ADD.$MSG_CONTEST?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])||isset($_SESSION[$OJ_NAME.'_'.'contest_creator'])){
?>
<li>
	<a class='btn btn-primary' href="contest_list.php" target="main"  title="<?php echo $MSG_HELP_CONTEST_LIST?>"><b><?php echo $MSG_CONTEST.$MSG_LIST?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?>
<li>
	<a class='btn btn-primary' href="team_generate.php" target="main" title="<?php echo $MSG_HELP_TEAMGENERATOR?>"><b><?php echo $MSG_TEAMGENERATOR?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])||isset( $_SESSION[$OJ_NAME.'_'.'password_setter'] )){
?><li>
	<a class='btn btn-primary' href="changepass.php" target="main" title="<?php echo $MSG_HELP_SETPASSWORD?>"><b><?php echo $MSG_SETPASSWORD?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><li>
	<a class='btn btn-primary' href="rejudge.php" target="main" title="<?php echo $MSG_HELP_REJUDGE?>"><b><?php echo $MSG_REJUDGE?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><li>
	<a class='btn btn-primary' href="privilege_add.php" target="main" title="<?php echo $MSG_HELP_ADD_PRIVILEGE?>"><b><?php echo $MSG_ADD.$MSG_PRIVILEGE?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><li>
	<a class='btn btn-primary' href="privilege_list.php" target="main" title="<?php echo $MSG_HELP_PRIVILEGE_LIST?>"><b><?php echo $MSG_PRIVILEGE.$MSG_LIST?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><li>
	<a class='btn btn-primary' href="problem_export.php" target="main" title="<?php echo $MSG_HELP_EXPORT_PROBLEM?>"><b><?php echo $MSG_EXPORT.$MSG_PROBLEM?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?><li>
	<a class='btn btn-primary' href="problem_import.php" target="main" title="<?php echo $MSG_HELP_IMPORT_PROBLEM?>"><b><?php echo $MSG_IMPORT.$MSG_PROBLEM?></b></a>
<?php }
if (isset($_SESSION[$OJ_NAME.'_'.'administrator'])){
?>
<!--<li>
	<a class='btn btn-primary' href="update_db.php" target="main" title="<?php echo $MSG_HELP_UPDATE_DATABASE?>"><b><?php echo $MSG_UPDATE_DATABASE?></b></a> -->
<?php }
if (isset($OJ_ONLINE)&&$OJ_ONLINE){
?><li>
	<a class='btn btn-primary' href="../online.php" target="main"><b><?php echo $MSG_ONLINE?></b></a>
<?php }
?>


<h4>
</body>
</html>
