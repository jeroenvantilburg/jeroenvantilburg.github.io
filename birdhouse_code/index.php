<!DOCTYPE html>
<html>
<head>
  <title>Bird box</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css">
<style>
.btn {
  background-color: DodgerBlue;
  border: none;
  color: white;
  padding: 12px 16px;
  margin: 5px 10px;
  font-size: 16px;
  font-family: system-ui;
  text-decoration: none;
  cursor: pointer;
  border-radius: 15px;
  min-width: 150px;
  /*min-height: 50px;*/
  display: inline-block;
}

/* Darker background on mouse-over */
.btn:hover {
  background-color: RoyalBlue;
}
</style>

</head>

<h2>Bird box</h2>
<a href="http://esp32-arduino.home" class="btn"><i class="fa fa-video-camera"></i> Live webcam</a>  (interval needs to be negative)

<br clear="all"/>
<a href="http://raspberrypi.home/esp32/gallery.php" class="btn"><i class="fa fa-file-image-o"></i> Gallery</a> 

<p/>

<?php
$interval = "";
if(isset($_GET['submit'])){
  $interval = $_GET['interval'];
  $fp = fopen('interval.txt', 'w');
  fwrite($fp, $interval);
  echo "The interval is changed to ".$interval." seconds";
  fclose($fp);
} else {
  $myfile = fopen("interval.txt", "r") or die("Unable to open file!");
  $interval = fread($myfile,filesize("interval.txt"));
  fclose($myfile);
}
?>

<body>
  <form method="get">
    Set the interval in seconds:<br>
    <input type="number" name="interval" value="<?php echo $interval;?>">
    <input type="submit" name="submit">
  </form>
</body>

</html>


