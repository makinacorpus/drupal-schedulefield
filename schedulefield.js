;(function($){

Drupal.behaviors.schedulefield = {
  attach: function(context, settings) {

    $('.schedulefield-datepicker', context).datepicker({
      'dateFormat' : settings.schedulefield.date_format
    });
  }
};
})(jQuery);
