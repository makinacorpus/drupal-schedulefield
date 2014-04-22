<?php

/**
 * Function helper to convert sql formats to drupal formats.
 *
 * @param $data
 *   A schedule data array.
 * @param $date_format
 *   A date format.
 * @param $time_format
 *   A time format.
 */
function schedulefield_convert_sql_to_php(&$data, $date_format, $time_format) {

  // Convert dates.
  $date_fields = array('date_start', 'date_end');
  foreach ($date_fields as $field) {
    if (isset($data[$field])) {
      $datetime = DateTime::createFromFormat(SCHEDULEFIELD_SQL_DATE_FORMAT, $data[$field]);
      $data[$field] = '';
      if ($datetime) {
        $data[$field] = $datetime->format($date_format);
      }
    }
  }

  // Convert times.
  $time_fields = array('time_start', 'time_end');
  foreach ($time_fields as $field) {
    foreach ($data['days'] as $day_week => &$periods) {
      foreach ($periods as $period => &$times) {

        if (isset($times[$field])) {
          $datetime = DateTime::createFromFormat(SCHEDULEFIELD_SQL_TIME_FORMAT, $times[$field]);
          $times[$field] = '';
          if ($datetime) {
            $times[$field] = $datetime->format($time_format);
          }
        }
      }
    }
  }
}

/**
 * Function helper to convert drupal formats to sql formats.
 *
 * @param $data
 *   An array of schedules.
 * @param $date_format
 *   A date format e.g : d-m-Y
 * @param $time_format
 *   A time format e.g : H:i.
 */
function schedulefield_convert_php_to_sql(&$data, $date_format, $time_format) {

  // Convert dates.
  $date_fields = array('date_start', 'date_end');
  foreach ($date_fields as $field) {
    if (isset($data[$field])) {

      $datetime = DateTime::createFromFormat($date_format, $data[$field]);
      $data[$field] = '';
      if ($datetime) {
        $data[$field] = $datetime->format(SCHEDULEFIELD_SQL_DATE_FORMAT);
      }
    }
  }

  // Convert times.
  $time_fields = array('time_start', 'time_end');
  foreach ($time_fields as $field) {
    foreach ($data['days'] as $day_week => &$periods) {
      foreach ($periods as $period => &$times) {

        if (isset($times[$field])) {

          $datetime = DateTime::createFromFormat($time_format, $times[$field]);
          $times[$field] = '';
          if ($datetime) {
            $times[$field] = $datetime->format(SCHEDULEFIELD_SQL_TIME_FORMAT);
          }
        }
      }
    }
  }
}

/**
 * Function helper to get dayname from a given day week number.
 */
function schedulefield_get_dayname($day_week) {
  $day_weeks = array(
    0 => t('Sunday'),
    1 => t('Monday'),
    2 => t('Tuesday'),
    3 => t('Wednesday'),
    4 => t('Thursday'),
    5 => t('Friday'),
    6 => t('Saturday'),
  );

  return isset($day_weeks[$day_week]) ? $day_weeks[$day_week] : '';
}