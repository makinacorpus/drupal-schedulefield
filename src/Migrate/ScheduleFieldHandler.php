<?php

namespace MakinaCorpus\Schedule\Migrate;

class ScheduleFieldHandler extends \MigrateFieldHandler
{
    public function __construct()
    {
        $this->registerTypes(['schedule']);
    }

    /**
     * Build interval from string
     *
     * @param string $string
     *
     * @return \DateInterval
     */
    private function buildInterval($string)
    {
        $items = explode(':', $string, 2);

        return new \DateInterval('PT' . $items[0] . 'H' . $items[1] . 'M');
    }

    public function prepare($entity, array $field_info, array $instance, array $values)
    {
        $return = null;
        $arguments = [];

        if (isset($values['days'])) {
            // We are NOT a multiple thingy
            $values = [$values];
        }

        $language = $this->getFieldLanguage($entity, $field_info, $arguments);

        $delta = 0;
        foreach ($values as $value) {

            if (!is_array($value) || !isset($value['days'])) {
                continue; // Invalid field
            }

            // We need to convert strings to \DateTime objects
            foreach ($value['days'] as $day => $slots) {
                foreach ($slots as $name => $data) {
                    $done = false;
                    foreach (['time_start', 'time_end'] as $foo) {
                        if (isset($data[$foo])) {
                            $value['days'][$day][$name][$foo] = $this->buildInterval($data[$foo]);
                            $done = true;
                        }
                    }
                    if (!$done) {
                        unset($value['days'][$day][$name]);
                    }
                }
            }

            $return[$language][$delta++] = $value;
        }

        return $return;
    }
}

