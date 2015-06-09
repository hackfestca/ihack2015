<?php

if(!isset($_COOKIE['lang'])) {
    $lang = 'fr';
    setcookie("lang", 'fr');
} else{
    $lang = $_COOKIE['lang'];
}

$dbconn = pg_connect("host=localhost port=5432 dbname=information user=postgres password=JessyIsAPainInTheAss") or die('Connexion impossible : ' . pg_last_error());
$query = "SELECT string,data FROM language where lang like '".$lang."'";
$result = pg_query($query) or die('Échec de la requête : ' . pg_last_error());


while ($line = pg_fetch_row($result)) {
    $line[0] == "hello" ? $hello = $line[1]: $text = $line[1];
    $line[0] == "text" ? $text = $line[1]: $hello = $line[1];
}

echo "<h1>".$hello."</h1>";
echo $text;

echo '<br><br> <a href="en.php">en</a>/<a href="fr.php">fr</a>';
pg_close($dbconn);

?>
