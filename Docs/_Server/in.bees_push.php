<?php

$outH = fopen('file:///tmp/php_Bees.out', 'w');

/* if started from commandline, wrap parameters to $_POST and $_GET */
if (!isset($_SERVER["HTTP_HOST"])) {
  $postAry = array();

  //parse_str($argv[1], $_GET);
  //parse_str($argv[1], $_POST);

  $usrPwd  = "bees:So§Let5MeIn";
  $auth    = "Authorization: Basic ";
  $auth   .= base64_encode($usrPwd);
  $auth   .= "\r\n";

  $inH = fopen('php://stdin','r');
  while (!feof($inH)) {
    $postAry[] = fgets($inH);
  }
  fclose($inH);

  $stat = print_r($postAry, TRUE);
  fputs($outH, $stat);

  $json = $postAry[9];
  $appAry = json_decode($json, TRUE);
  $stat = print_r($appAry, TRUE);
  fputs($outH, $stat);

  //fputs($outH, $auth);

  if (!strcmp($auth, $postAry[4])) {
    fputs($outH, "User/PWD OK\r\n");

    //string openssl_encrypt ( string $data , string $method , string $key [, int $options = 0 [, string $iv = "" [, string &$tag = NULL [, string $aad = "" [, int $tag_length = 16 ]]]]] );
    $tsKeyCmp     = ((0x0392ea84 + ($appAry['payload_fields']['mote_ts'] & 0x7fffffff)) % 0x17ef23ab) ^ ((0x339dc627 + ($appAry['payload_fields']['mote_ts'] & 0x07ffffff) * 7) % 0x2a29e2cc);
//decoded.mote_cm = ((0x0392ea84 + (                     decoded.mote_ts & 0x7fffffff)) % 0x17ef23ab) ^ ((0x339dc627 + (                     decoded.mote_ts & 0x07ffffff) * 7) % 0x2a29e2cc);

    //fprintf($outH, "tsKeyCM  = %x\r\n", $appAry['payload_fields']['mote_cm']);
    //fprintf($outH, "tsKeyCmp = %x\r\n", $tsKeyCmp);

    if (($appAry['payload_fields']['mote_cm'] == $tsKeyCmp) && $appAry['payload_fields']['mote_flags']) {
      /* Database access */
      {
        fputs($outH, "UNLOCKED - now processing data\r\n");

        // Database access
        $dbconn = pg_connect("host=localhost dbname=bees user=bees_push password=Push1And.Go");

        // First: handle devs table
        $query = "SELECT id, dev_id  FROM devs  WHERE dev_eui = '" . $appAry['hardware_serial'] . "';";
        $result = pg_query($query);
        if (!$result) {
          fprintf($outH, "SQL-Error: %s\r\n", pg_last_error());
          fprintf($outH, "Query was: %s\r\n", $query);
          die('PQSQL query error');
        }

        $rc = pg_num_rows($result);
        if ($rc == 0) {
          pg_free_result($result);

          // Insert new device
          $query = "INSERT INTO devs(dev_id, dev_eui, app_id)  VALUES('" . $appAry['dev_id'] . "', '" . $appAry['hardware_serial'] . "', '" . $appAry['app_id'] . "');";
          $result = pg_query($query);
          fprintf($outH, "SQL-Error: %s\r\n", pg_last_error());
          fprintf($outH, "Query was: %s\r\n", $query);

        } else {
          // Get Device IDs
          $line                 = pg_fetch_array($result, null, PGSQL_ASSOC);
          $db_dev__id           = $line['id'];
          $db_dev__dev_id       = $line['dev_id'];

          fprintf($outH, "compare old dev_id=%s with new dev_id=%s\r\n", $db_dev__dev_id, $appAry['dev_id']);

          if (strcmp($db_dev__dev_id, $appAry['dev_id'])) {
            pg_free_result($result);

            // Update dev_id and timestamp
            $query = "UPDATE devs  SET ts_last = now(), dev_id = '" . $appAry['dev_id'] . "'  WHERE id = " . $db_dev__id . ";";
            $result = pg_query($query);
            fprintf($outH, "SQL-Error: %s\r\n", pg_last_error());
            fprintf($outH, "Query was: %s\r\n", $query);

          } else {
            // Update timestamp only
            $query = "UPDATE devs  SET ts_last = now()  WHERE id = " . $db_dev__id . ";";
            $result = pg_query($query);
            fprintf($outH, "SQL-Error: %s\r\n", pg_last_error());
            fprintf($outH, "Query was: %s\r\n", $query);
          }
        }
        pg_free_result($result);

        // Second: handle msgs table
        $db_msgs__ad_ch0_raw  = $appAry['payload_fields']['ad_ch0_raw'];
        $db_msgs__ad_ch1_raw  = $appAry['payload_fields']['ad_ch1_raw'];
        $db_msgs__ad_ch2_raw  = $appAry['payload_fields']['ad_ch2_raw'];
        $db_msgs__ad_ch3_raw  = $appAry['payload_fields']['ad_ch3_raw'];
        $db_msgs__ad_ch4_raw  = $appAry['payload_fields']['ad_ch4_raw'];
        $db_msgs__ad_ch5_raw  = $appAry['payload_fields']['ad_ch5_raw'];
        $db_msgs__ad_ch6_raw  = $appAry['payload_fields']['ad_ch6_raw'];
        $db_msgs__ad_ch7_raw  = $appAry['payload_fields']['ad_ch7_raw'];
        $db_msgs__i_bat       = $appAry['payload_fields']['i_bat'];
        $db_msgs__v_bat       = $appAry['payload_fields']['v_bat'];
        $db_msgs__v_aux1      = $appAry['payload_fields']['v_aux1'];
        $db_msgs__v_aux2      = $appAry['payload_fields']['v_aux2'];
        $db_msgs__v_aux3      = $appAry['payload_fields']['v_aux3'];
        $db_msgs__wx_temp     = $appAry['payload_fields']['wx_temp'];
        $db_msgs__wx_rh       = $appAry['payload_fields']['wx_rh'];
        $db_msgs__wx_baro_a   = $appAry['payload_fields']['wx_baro_a'];
	$db_msgs__gnss_lat    = $appAry['payload_fields']['latitude'];
        $db_msgs__gnss_lon    = $appAry['payload_fields']['longitude'];
        $db_msgs__gnss_alt    = $appAry['payload_fields']['altitude'];
        $db_msgs__gnss_acc    = $appAry['payload_fields']['accuracy'];
        $db_msgs__gnss_spd    = $appAry['payload_fields']['speed'];
        $db_msgs__gnss_crs    = $appAry['payload_fields']['course'];
        $db_msgs__mote_flags  = $appAry['payload_fields']['mote_flags'];
        $db_msgs__mote_ts     = $appAry['payload_fields']['mote_ts'];
        $db_msgs__mote_ctr_up = $appAry['counter'];
        $db_msgs__gw_frq      = $appAry['metadata']['frequency'];
        $db_msgs__gw_sf       = $appAry['metadata']['data_rate'];
        $db_msgs__gw_cnt      = count($appAry['metadata']['gateways']);
        //$db_msgs__gw_id       = $appAry['metadata']['gateways'][0]['gtw_id'];
        //$db_msgs__gw_dbm      = $appAry['metadata']['gateways'][0]['rssi'];
        //$db_msgs__gw_snr      = $appAry['metadata']['gateways'][0]['snr'];
        //$db_msgs__gw_lat      = $appAry['metadata']['gateways'][0]['latitude'];
        //$db_msgs__gw_lon      = $appAry['metadata']['gateways'][0]['longitude'];
        //$db_msgs__gw_alt      = $appAry['metadata']['gateways'][0]['altitude'];

	$db_msgs__i_bat       = 0.0 + $db_msgs__i_bat;
	$db_msgs__gw_alt      = 0.0 + $db_msgs__gw_alt;
        $db_msgs__gnss_spd    = 0.0 + $db_msgs__gnss_spd;
        $db_msgs__gnss_crs    = 0.0 + $db_msgs__gnss_crs;


        $query  = "INSERT INTO msgs(devs_fk, ad_ch0_raw, ad_ch1_raw, ad_ch2_raw, ad_ch3_raw, ad_ch4_raw, ad_ch5_raw, ad_ch6_raw, ad_ch7_raw, i_bat, v_bat, v_aux1, v_aux2, v_aux3, wx_temp, wx_rh, wx_baro_a, gnss_lat, gnss_lon, gnss_alt_m, gnss_acc_m, gnss_spd, gnss_crs, mote_flags, mote_ts, mote_ctr_up, gw_frq, gw_sf, gw_cnt, gw_id_ary, gw_dbm_ary, gw_snr_ary, gw_lat_ary, gw_lon_ary, gw_alt_ary)  ";

        $query .= "VALUES(" . $db_dev__id . ", " . $db_msgs__ad_ch0_raw . ", " . $db_msgs__ad_ch1_raw . ", " . $db_msgs__ad_ch2_raw . ", " . $db_msgs__ad_ch3_raw . ", " . $db_msgs__ad_ch4_raw . ", " . $db_msgs__ad_ch5_raw . ", " . $db_msgs__ad_ch6_raw . ", " . $db_msgs__ad_ch7_raw . ", ";
        $query .= $db_msgs__i_bat . ", " . $db_msgs__v_bat . ", " . $db_msgs__v_aux1 . ", " . $db_msgs__v_aux2 . ", " . $db_msgs__v_aux3 . ", ";
	$query .= $db_msgs__wx_temp . ", " . $db_msgs__wx_rh . ", " . $db_msgs__wx_baro_a . ", ";
	$query .= $db_msgs__gnss_lat . ", " . $db_msgs__gnss_lon . ", " . $db_msgs__gnss_alt . ", " . $db_msgs__gnss_acc . ", " . $db_msgs__gnss_spd . ", " . $db_msgs__gnss_crs . ", ";
	$query .= $db_msgs__mote_flags . ", to_timestamp(" . $db_msgs__mote_ts . " / 1000), " . $db_msgs__mote_ctr_up . ", ";
        $query .= $db_msgs__gw_frq . ", '" . $db_msgs__gw_sf . "', ";
	$query .= $db_msgs__gw_cnt . ", ";

	$query .= "'{";
	for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
	  $query .= "\"" . $appAry['metadata']['gateways'][$gwIdx - 1]['gtw_id'] . "\"";
	}
	$query .= "}', ";

        $query .= "'{";
        for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
          $query .= (0 + $appAry['metadata']['gateways'][$gwIdx - 1]['rssi']);
        }
        $query .= "}', ";

        $query .= "'{";
        for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
          $query .= (0 + $appAry['metadata']['gateways'][$gwIdx - 1]['snr']);
        }
        $query .= "}', ";

        $query .= "'{";
        for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
          $query .= (0 + $appAry['metadata']['gateways'][$gwIdx - 1]['latitude']);
        }
        $query .= "}', ";

        $query .= "'{";
        for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
          $query .= (0 + $appAry['metadata']['gateways'][$gwIdx - 1]['longitude']);
        }
        $query .= "}', ";

        $query .= "'{";
        for ($gwIdx = 1; $gwIdx <= $db_msgs__gw_cnt; $gwIdx++) {
          if ($gwIdx > 1) {
            $query .= ", ";
          }
          $query .= (0 + $appAry['metadata']['gateways'][$gwIdx - 1]['altitude']);
        }
        $query .= "}'";

        $query .= ");";

        $result = pg_query($query);
	if(!$result) {
          fprintf($outH, "SQL-Error: %s\r\n", pg_last_error());
          fprintf($outH, "Query was: %s\r\n", $query);
  	}

        pg_free_result($result);
        pg_close($dbconn);
      }
    }


    /* LoRa track_me application to APRS gateway */
    {
      // Get JSON Gateway data, when needed:
      // http://noc.thethingsnetwork.org:8085/api/v2/gateways/eui-b827ebfffe8e1e9f
      // Replace by GW EUI to look for.

      $PM_APRS_TX_HTTP_L1         = "POST / HTTP/1.1\r\n";
      $PM_APRS_TX_HTTP_L2         = "Content-Length: %d\r\n";
      $PM_APRS_TX_HTTP_L3         = "Content-Type: application/octet-stream\r\n";
      $PM_APRS_TX_HTTP_L4         = "Accept-Type: text/plain\r\n\r\n";
      $PM_APRS_TX_LOGIN           = "user %s pass %s vers %s %s\r\n";
      $PM_APRS_TX_FORWARD         = "%s>APRS,TCPIP*:";                              // USER, SSID with prefixing "-"
      $PM_APRS_TX_SYMBOL_TABLE_ID = "/";
      $PM_APRS_TX_SYMBOL_CODE     = "j";  // /j: Jeep, /R: RV, \k: SUV, /W: WX
      $PM_APRS_TX_POS_SPD_HDG     = "!%02d%5.2f%s%s%03d%5.2f%s%s%03d/%03d";
      //$PM_APRS_TX_N1              = "%cGx=%+06.1fd Gy=%+06.1fd Gz=%+06.1fd";
      //$PM_APRS_TX_N2              = "%cAx=%+6.3fg Ay=%+6.3fg Az=%+6.3fg";
      //$PM_APRS_TX_N3              = "%cMx=%+6.1fuT My=%+6.1fuT Mz=%+6.1fuT";
      //$PM_APRS_TX_N4              = "%c/A=%06ld DP=%+5.2fC QNH=%7.2fhPa";
      $PM_APRS_TX_MSGEND          = "\r\n";

      /* Prepare data */
      $aprs_source_callsign = "DF4IAH-9";
      $app__lat = $appAry['payload_fields']['latitude'];
      $app__lon = $appAry['payload_fields']['longitude'];
      $app__alt = $appAry['payload_fields']['altitude'];
      $app__spd = $appAry['payload_fields']['speed'];
      $app__crs = $appAry['payload_fields']['course'];

      //fprintf($outH, 'hardware_serial = %s\r\n', $appAry['hardware_serial']);

      if (strcmp($appAry['hardware_serial'], '4934427330303031') == 0) {
        $aprs_source_callsign = "DF4IAH-11";
        $PM_APRS_TX_SYMBOL_TABLE_ID = "\\";
        $PM_APRS_TX_SYMBOL_CODE     = "k";  // /j: Jeep, /R: RV, \k: SUV, /W: WX
      }
      else if (strcmp($appAry['hardware_serial'], '4934427330303032') == 0) {
        $aprs_source_callsign = "DF4IAH-12";
        $PM_APRS_TX_SYMBOL_TABLE_ID = "/";
        $PM_APRS_TX_SYMBOL_CODE     = "j";  // /j: Jeep, /R: RV, \k: SUV, /W: WX
      }
      else if (strcmp($appAry['hardware_serial'], '4934427330303033') == 0) {
        $aprs_source_callsign = "DF4IAH-13";
        $PM_APRS_TX_SYMBOL_TABLE_ID = "/";
        $PM_APRS_TX_SYMBOL_CODE     = "W";  // /j: Jeep, /R: RV, \k: SUV, /W: WX
      }
      else if (strcmp($appAry['hardware_serial'], '4934427330303034') == 0) {
        $aprs_source_callsign = "DF4IAH-14";
        $PM_APRS_TX_SYMBOL_TABLE_ID = "/";
        $PM_APRS_TX_SYMBOL_CODE     = "R";  // /j: Jeep, /R: RV, \k: SUV, /W: WX
      }

      if ($app__lat >= 0) {
        $l_lat_deg_d = floor( $app__lat);
        $l_lat_minutes_f = 60 * ($app__lat - $l_lat_deg_d);
        $l_lat_hemisphere = "N";

      } else {
        $l_lat_deg_d = floor(-$app__lat);
        $l_lat_minutes_f = 60 * (-$app__lat - $l_lat_deg_d);
        $l_lat_hemisphere = "S";
      }

      if ($app__lon >= 0) {
        $l_lon_deg_d = floor( $app__lon);
        $l_lon_minutes_f = 60 * ($app__lon - $l_lon_deg_d);
        $l_lon_hemisphere = "E";

      } else {
        $l_lon_deg_d = floor(-$app__lon);
        $l_lon_minutes_f = 60 * (-$app__lon - $l_lon_deg_d);
        $l_lon_hemisphere = "W";
      }

      $l_course_deg = $app__crs;
      $l_speed_kn   = $app__spd * 3.6 / 1.852;  // m/s --> kn

      /* APRS constants */
      $g_aprs_login_user          = "DF4IAH";
      $g_aprs_login_pwd           = "17061";
      $APPLICATION_NAME           = "LoRa-IoT4Bees_APRS-GW";
      $APPLICATION_VERSION        = "1.0";

      /* Message content APRS */
      $aprs_content_msg  = sprintf($PM_APRS_TX_FORWARD, $aprs_source_callsign);
      $aprs_content_msg .= sprintf($PM_APRS_TX_POS_SPD_HDG, $l_lat_deg_d, $l_lat_minutes_f, $l_lat_hemisphere, $PM_APRS_TX_SYMBOL_TABLE_ID, $l_lon_deg_d, $l_lon_minutes_f, $l_lon_hemisphere, $PM_APRS_TX_SYMBOL_CODE, $l_course_deg, $l_speed_kn);
    //$aprs_content_msg .= sprintf($PM_APRS_TX_N1, $l_mark, $l_aprs_alert_1_gyro_x_mdps / 1000.f, $l_aprs_alert_1_gyro_y_mdps / 1000.f, $l_aprs_alert_1_gyro_z_mdps / 1000.f);
    //$aprs_content_msg .= sprintf($PM_APRS_TX_N2, $l_mark, $l_aprs_alert_1_accel_x_mg / 1000.f, $l_aprs_alert_1_accel_y_mg / 1000.f, $l_aprs_alert_1_accel_z_mg / 1000.f);
    //$aprs_content_msg .= sprintf($PM_APRS_TX_N3, $l_mark, $l_aprs_alert_2_mag_x_nT / 1000.f, $l_aprs_alert_2_mag_y_nT / 1000.f, $l_aprs_alert_2_mag_z_nT / 1000.f);
    //$aprs_content_msg .= sprintf($PM_APRS_TX_N4, $l_mark, $l_gns_msl_alt_ft, $l_twi1_hygro_DP_100 / 100.f, $l_twi1_baro_p_h_100 / 100.f);
      $aprs_content_msg .= $PM_APRS_TX_MSGEND;

      $aprs_content_hdr = sprintf($PM_APRS_TX_LOGIN, $g_aprs_login_user, $g_aprs_login_pwd, $APPLICATION_NAME, $APPLICATION_VERSION);
      $len = strlen($aprs_content_hdr) + 2 + strlen($aprs_content_msg);

      /* Line 1 */
      $aprs_msg  = sprintf($PM_APRS_TX_HTTP_L1);

      /* Line 2 */
      $aprs_msg .= sprintf($PM_APRS_TX_HTTP_L2, $len);

      /* Line 3 */
      $aprs_msg .= sprintf($PM_APRS_TX_HTTP_L3);

      /* Line 4 */
      $aprs_msg .= sprintf($PM_APRS_TX_HTTP_L4);

      /* Content header - authentication */
      $aprs_msg .= $aprs_content_hdr;

      /* Content message */
      $aprs_msg .= $aprs_content_msg;

      /* Ending line */
      $aprs_msg .= $PM_APRS_TX_MSGEND;

      /* Print APRS message for debugging */
      $dbgH = fopen('file:///tmp/php_Bees_APRS.out', 'w');
      fputs($dbgH, $aprs_msg);
      fclose($dbgH);

      /* Open destination port */
      $sockH = stream_socket_client("tcp://nuremberg.aprs2.net:8080", $errno, $errstr, 5);
      fputs($sockH, $aprs_msg);
      fclose($sockH);
    }
  }
}
fclose($outH);

?>
