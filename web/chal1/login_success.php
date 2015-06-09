<?php
session_save_path("/var/www/html/chal1/session");
session_start();
if(!isset( $_SESSION['myusername'])){
header("location:login.php");
}
?>

<html>
<body>
Login Successful
</body>
</html>
