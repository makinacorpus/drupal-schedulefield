<?php

/**
 * @file
 * Implement a schedule field.
 */

/**
 * Implements hook_field_info().
 */
function schedulefield_field_info() {
  return array(
    'schedule' => array(
      'label' => t('Schedule'),
      'description' => t('This field stores relative data about scheduling.'),
      'settings' => array(),
      'instance_settings' => array(),
      'default_widget' => 'schedule',
      'default_formatter' => 'schedule',
    ),
  );
}

/**
 * Implements hook_field_is_empty();
 */
function schedulefield_field_is_empty($item, $field) {
  // Have at least one date so not empty.
  if (!empty($item['date_start']) || !empty($item['date_end'])) {
    return FALSE;
  }

  // No dates but at least one time, not empty.
  if (!empty($item['days'])) {
    foreach ($item['days'] as $day => $slots) {
      foreach ($slots as $slot => $times) {
        if (!empty($times['time_start']) || !empty($times['time_end'])) {
          return FALSE;
        }
      }
    }
  }

  // Otherwise, field is empty.
  return TRUE;
}

/**
 * Implements hook_field_presave().
 */
function schedulefield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  global $language;

  foreach ($items as &$item) {

    $pid = 0;

    // Redondant check if submitted through a form but not if submitted
    // programmatically. No required field means no update necessary.
    if (!schedulefield_field_is_empty($item, $field)) {

      // Schedule update ?
      if (!empty($item['pid'])) {
        schedulefield_schedule_update($item);
        $pid = $item['pid'];
      } else {
        $pid = schedulefield_schedule_insert($item);
      }
    }

    $item = array('pid' => $pid);
  }
}

/**
 * Implements hook_field_delete().
 */
function schedulefield_field_delete($entity_type, $entity, $field, $instance, $langcode, $items) {

  $pids = array();
  foreach ($items as $item) {
    $pids[$item['pid']] = $item['pid'];
  }
  schedulefield_schedule_delete_multiple($pids);
}
