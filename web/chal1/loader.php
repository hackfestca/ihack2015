<?php
session_save_path("/var/www/html/chal1/session");
session_start();
$_SESSION["myusername"] = "user";
?>
