<?php
session_save_path("/var/www/html/chal1/session");
session_start();
if(!isset( $_SESSION['myusername'])){
	header("location:login.php");
	exit();
}
if($_SESSION['myusername'] == "admin"){
	echo "hey you got admin login : FLAG-69b11d35af9e229ccd58808cbac10224";
}
if($_SESSION['myusername'] == "user"){
        echo "hey you got user login ! that's bad you need admin. You are near of the flag";
}



?>


