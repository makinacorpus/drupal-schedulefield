<?php

/**
 * Implements hook_field_widget_info().
 */
function schedulefield_field_widget_info() {
  return [
    'schedule' => [
      'label'       => t('Schedule'),
      'field types' => ['schedule'],
      'settings'    => [
        'collect_text'    => false,
        'collect_period'  => false,
      ],
    ],
  ];
}

/**
 * Implements hook_field_widget_settings_form().
 */
function schedulefield_field_widget_settings_form($field, $instance) {
  $form = [];

  $settings = $instance['widget']['settings'];

  $form['collect_period'] = [
    '#type'           => 'checkbox',
    '#title'          => t('Collect period (leave empty if uncheck)'),
    '#default_value'  => $settings['collect_period'],
  ];
  $form['collect_text'] = [
    '#type'           => 'checkbox',
    '#title'          => t('Collect start and end text'),
    '#default_value'  => $settings['collect_text'],
  ];

  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function schedulefield_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $settings = $instance['widget']['settings'];

  $element += [
    '#type'             => 'fieldset',
    '#title'            => t('Schedule'),
    '#collapsible'      => false,
    '#tree'             => true,
    '#element_validate' => ['schedulefield_validate_dates'],
  ];

  if (!empty($items[$delta]['pid'])) {
    $element['pid'] = [
      '#type'   => 'value',
      '#value'  => $items[$delta]['pid'],
    ];
  }

  // Period start and stop dates.
  $element['date_start'] = [
    '#type'             => 'udate',
    '#title'            => t('Date start'),
    '#default_value'    => !empty($items[$delta]['date_start']) ? $items[$delta]['date_start'] : '',
  ];
  $element['date_end'] = [
    '#type'             => 'udate',
    '#title'            => t('Date end'),
    '#default_value'    => !empty($items[$delta]['date_end']) ? $items[$delta]['date_end'] : '',
  ];

  if (!empty($settings['collect_text'])) {
    $element['text_opening'] = [
      '#type'           => 'textfield',
      '#title'          => t('Opening text'),
      '#default_value'  => isset($items[$delta]['text_opening']) ? $items[$delta]['text_opening'] : '',
      '#maxlength'      => 255,
    ];
    $element['text_closing'] = [
      '#type'           => 'textfield',
      '#title'          => t('Closing text'),
      '#default_value'  => isset($items[$delta]['text_closing']) ? $items[$delta]['text_closing'] : '',
      '#maxlength'      => 255,
    ];
  }

  $slots = schedulefield_get_slots();
  $first_day = variable_get('date_first_day', 0);
  $days_week = range(0, 6);
  if ($first_day !== 0) {
    $previous_days = array_splice($days_week, 0, $first_day);
    $days_week = array_merge($days_week, $previous_days);
  }

  foreach ($days_week as $day_week) {

    $element['days'][$day_week] = [
      '#type'  => 'item',
      '#title' => schedulefield_get_day_name($day_week),
    ];

    foreach ($slots as $slot => $slot_name) {

      if (!empty($items[$delta]['days'][$day_week][$slot]['did'])) {
        $element['days'][$day_week][$slot]['did'] = [
          '#type'   => 'value',
          '#value'  => $items[$delta]['days'][$day_week][$slot]['did'],
        ];
      }

      $element['days'][$day_week][$slot] = [
        '#prefix' => '<div class="form-inline">',
        '#suffix' => '</div>',
      ];
      $element['days'][$day_week][$slot]['time_start'] = [
        '#type'             => 'utime',
        '#attributes'       => ['title' => t('Time start')],
        '#field_prefix'     => t('From'),
        '#granularity'      => UDATE_GRANULARITY_MINUTE,
        '#default_value'    => !empty($items[$delta]['days'][$day_week][$slot]['time_start']) ? $items[$delta]['days'][$day_week][$slot]['time_start'] : '',
      ];
      $element['days'][$day_week][$slot]['time_end'] = [
        '#type'             => 'utime',
        '#attributes'       => ['title' => t('Time end')],
        '#field_prefix'     => t('To'),
        '#granularity'      => UDATE_GRANULARITY_MINUTE,
        '#default_value'    => !empty($items[$delta]['days'][$day_week][$slot]['time_end']) ? $items[$delta]['days'][$day_week][$slot]['time_end'] : '',
      ];

      $element['days'][$day_week][$slot]['#element_validate'][] = 'schedulefield_validate_times';
    }
  }

  return $element;
}

/**
 * Date elements validate.
 */
function schedulefield_validate_dates($element, &$form_state, $form) {
  if (!form_get_error($element['date_start']) &&
      !form_get_error($element['date_end']) &&
      !empty($element['date_start']['#value']) &&
      !empty($element['date_end']['#value'])
  ) {
    if ($element['date_start']['#value'] > $element['date_end']['#value']) {
      form_error($element, t('Field %start could not be greater than field %end.', array('%start' => $element['date_start']['#title'], '%end' => $element['date_end']['#title'])));
    }
  }
}

/**
 * Time elements validate.
 */
function schedulefield_validate_times($element, &$form_state, $form) {
  if (!form_get_error($element['time_start']) &&
      !form_get_error($element['time_end']) &&
      !empty($element['time_start']['#value']) &&
      !empty($element['time_end']['#value'])
  ) {
    if ($element['time_start']['#value'] > $element['time_end']['#value']) {
      form_error($element, t('Field %start could not be greater than field %end.', array('%start' => $element['time_start']['#title'], '%end' => $element['time_end']['#title'])));
    }
  }
}
