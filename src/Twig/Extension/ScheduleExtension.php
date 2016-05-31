<?php

namespace MakinaCorpus\Schedule\Twig\Extension;

class ScheduleExtension extends \Twig_Extension
{
    public function getFunctions()
    {
        return [
            new \Twig_SimpleFunction(
                'schedulefield_get_day_name', 'schedulefield_get_day_name', ['is_safe' => ['html']]
            ),
            new \Twig_SimpleFunction(
                'schedulefield_get_day_microdata_url',
                'schedulefield_get_day_microdata_url',
                ['is_safe' => ['html']]
            ),
        ];
    }

    /**
     * Returns the name of the extension.
     *
     * @return string The extension name
     */
    public function getName()
    {
        return 'drupal_schedulefield';
    }
}
