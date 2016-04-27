<?php

/**
 * Implements hook_field_formatter_info().
 */
function schedulefield_field_formatter_info() {
  return [
    'schedule' => [
      'label'       => t('Schedule'),
      'field types' => ['schedule'],
    ],
  ];
}

/**
 * Implements hook_field_formatter_settings_form();
 */
function schedulefield_field_formatter_settings_form($field, $instance, $view_mode, $form, $form_state) {
  $element = [];

  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $formats = system_get_date_types();
  $options = array();
  foreach ($formats as $format) {
    $options[$format['type']] = $format['title'];
  }

  $element['date_format'] = array(
    '#type' => 'select',
    '#title' => t('Date format'),
    '#options' => $options,
    '#default_value' => $settings['date_format'],
  );

  $element['time_format'] = array(
    '#type' => 'select',
    '#title' => t('Time format'),
    '#options' => $options,
    '#default_value' => $settings['time_format'],
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary();
 */
function schedulefield_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $summary = array();

  $types = system_get_date_types();
  $summary[] = t('Date format : %format', array('%format' => $types[$settings['date_format']]['title']));
  $summary[] = t('Time format : %format', array('%format' => $types[$settings['time_format']]['title']));

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_prepare_view().
 */
function schedulefield_field_prepare_view($entity_type, $entities, $field, $instances, $langcode, &$items) {

  // Preload all schedules.
  $pids = array();
  foreach ($items as $entity_id => $item) {
    foreach ($item as $delta => $values) {
      $pids[$values['pid']] = $values['pid'];
    }
  }
  $schedules = schedulefield_schedule_load_multiple($pids);

  foreach ($items as &$item) {
    foreach ($item as $delta => &$values) {
      $values += $schedules[$values['pid']];
    }
  }
}

/**
 * Implements hook_field_formatter_view().
 */
function schedulefield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  return [];
  // FIXME
  /*
  global $language;
  $element = array();

  foreach ($items as $delta => $item) {

    // Convert SQL format to drupal formats.

    // Re-order days.
    if (!empty($items['days'])) {
      ksort($items['days']);
      if ($first_day > 0) {
        foreach ($items['days'] as $day_week => $values) {
          if ($day_week >= $first_day) {
            break;
          }
          $items['days'][$day_week] = $values;
        }
      }
    }

    $element[$delta] = array(
      '#theme' => 'schedulefield_formatter',
      '#item' => $item,
    );
  }

  return $element;
   */
}

/**
 * Returns HTML for a default schedule field.
 *
 * @ingroup themeable
 */
function theme_schedulefield_formatter($variables) {
  $item = $variables['item'];
  $output = '';

  $output .= '<div class="schedulefield">';
  if (isset($item['date_start']) && isset($item['date_end'])) {
    $output .= '<p>' . t('From !start to !end', array('!start' => $item['date_start'], '!end' => $item['date_end']));
  }
  elseif (isset($item['date_start'])) {
    $output .= '<p>' . t('From !start', array('!start' => $item['date_start']));
  }
  elseif (isset($item['date_end'])) {
    $output .= '<p>' . t('To !end', array('!end' => $item['date_end']));
  }

  if (!empty($item['text_opening'])) {
    $output .= '<br />' . check_plain($item['text_opening']);
  }
  if (!empty($item['text_closing'])) {
    $output .= '<br />' . check_plain($item['text_closing']);
  }

  foreach ($item['days'] as $day_week => $periods) {
    $output .= '<br /><br />' . schedulefield_get_dayname($day_week) . ' : ';
    $days = array();
    foreach ($periods as $period => $datetimes) {

      if (isset($datetimes['time_start']) && isset($datetimes['time_end'])) {
        $days[] = t('!period : From !time_start to !time_end', array('!period' => ucfirst($period), '!time_start' => $datetimes['time_start'], '!time_end' => $datetimes['time_end']));
      }
      elseif (isset($datetimes['time_start'])) {
        $days[] = t('!period : From !time_start', array('!period' => ucfirst($period), '!time_start' => $datetimes['time_start']));
      }
      elseif (isset($datetimes['time_end'])) {
        $days[] = t('!period : To !time_end', array('!period' => ucfirst($period), '!time_end' => $datetimes['time_end']));
      }
    }
    $output .= implode(', ', $days);
  }

  $output .= '</p></div>';
  return $output;
}