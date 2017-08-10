<?php


/**
 * Implements hook_field_formatter_info().
 */
function commerce_express_checkout_field_formatter_info() {

  $formatter['commerce_express_checkout_link'] = array(
    'label' => t('Express Checkout Link'),
    'description' => t('Display the Express Checkout link'),
    'field types' => array('commerce_product_reference'),
    'settings' => array(
      'text_to_display' => 'Express checkout',
      'class' => '',
      'line_item_type' => 'product',
    ),
  );

  // todo: The code for this formatter method is incomplete.
  $formatter['commerce_express_checkout_select'] = array(
    'label' => t('Express Checkout Select'),
    'description' => t('Display the Express Checkout as select options.
      Useful when there is more than one product in the page.'),
    'field types' => array('commerce_product_reference'),
    'settings' => array(
      'class' => '',
      'line_item_type' => 'product',
    ),
  );

  return $formatter;
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function commerce_express_checkout_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = array_merge(field_info_formatter_settings($display['type']), $display['settings']);

  $element = array();

  switch ($display['type']) {
    case 'commerce_express_checkout_link':
    case 'commerce_express_checkout_select':
      $element['class'] = array(
        '#type' => 'textfield',
        '#title' => t('CSS class'),
        '#default_value' => $settings['class'],
        '#description' => t('Additional css class separate by blank.'),
      );

      $line_item_types = commerce_line_item_types();
      $options = array();
      foreach ($line_item_types as $type => $line_item_type) {
        $options[$type] = $line_item_type['name'];
      }
      $element['line_item_type'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => $settings['line_item_type'],
        '#description' => t('Select line item type.'),
      );

      // There is a single product and we can use this option to change the
      // display text of the button link.
      $element['text_to_display'] = array(
        '#type' => 'textfield',
        '#title' => t('Text to display'),
        '#default_value' => $settings['text_to_display'],
        '#description' => t('Set the button text.'),
      );
      break;

  }

  return $element;
}


/**
 * Implements hook_field_formatter_settings_summary().
 */
function commerce_express_checkout_field_formatter_settings_summary($field, $instance, $view_mode) {

  $display = $instance['display'][$view_mode];
  $settings = array_merge(field_info_formatter_settings($display['type']), $display['settings']);

  $summary = array();
  switch($display['type']) {
    case 'commerce_express_checkout_link':
    case 'commerce_express_checkout_select':
      $summary = array(
        t('Text to display: !text', array('!text' => !empty($settings['text_to_display']) ? $settings['text_to_display'] : t('Express checkout'))),
        t('CSS class: !css', array('!css' => !empty($settings['class']) ? $settings['class'] : '')),
        t('Line item type: !line_item_type', array('!line_item_type' => !empty($settings['line_item_type']) ? $settings['line_item_type'] : '')),
      );
      break;
  }

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_formatter_view().
 */
function commerce_express_checkout_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();

  $settings = array_merge(field_info_formatter_settings($display['type']), $display['settings']);

  switch ($display['type']) {
    case 'commerce_express_checkout_link':
      // Display the first (the only one) value as express checkout link.
      $element = commerce_express_checkout_field_formatter_view_as_link(
        $entity_type, $entity, $field, $instance, $langcode, $items, $display);

      break;

    case 'commerce_express_checkout_select':
      // Display the values as radio buttons with a submit button.
      $element = commerce_express_checkout_field_formatter_view_as_radios(
        $entity_type, $entity, $field, $instance, $langcode, $items, $display);

      break;
  }

  return $element;
}


/**
 * View the express checkout as link.
 */
function commerce_express_checkout_field_formatter_view_as_link($entity_type,
  $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();

  $settings = $display['settings'];

  $attributes = array();

  if (!empty($settings['class'])) {
    $attributes['class'] = $settings['class'];
  }

  list($entity_id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);

  // Get the first item to render
  $item = reset($items);

  if (!empty($item['product_id'])) {
    $options = array(
      'entity_id' => $entity_id,
      'entity_type' => $entity_type,
      'bundle' => $bundle,
      'quantity' => 1,
      'line_item_type' => $settings['line_item_type'],
    );

    $element[0]['#markup'] = commerce_express_checkout_generate_checkout_button(
      $item['product_id'], $settings['text_to_display'], $attributes, $options);
  }

  return $element;
}

/**
 * View the express checkout as radios and a checkout button.
 */
function commerce_express_checkout_field_formatter_view_as_radios($entity_type,
  $entity, $field, $instance, $langcode, $items, $display) {

  $element = array();
  $settings = $display['settings'];

  $attributes = array();
  if (!empty($settings['class'])) {
    $attributes['class'] = $settings['class'];
  }

  $product_ids = array();
  foreach ($items as $delta => $item) {
    $product_ids[$item['product_id']] = $item['product_id'];
  }

  if (!empty($product_ids)) {
    // Load the referenced products.
    $products = commerce_product_load_multiple($product_ids, array('status' => 1));

    $element[] = array(
      '#arguments' => array(
        'form_id' => 'commerce_express_checkout_express_checkout_form',
        'products' => $products,
        'display' => $display,
        'line_item_type' => $settings['line_item_type']
      ),
    );

    //$element[] = drupal_get_form('commerce_express_checkout_express_checkout_form',
    //  $entity_type, $entity, $products, $display);
  }

  return $element;
}

/**
 * Implements hook_field_attach_view_alter().
 *
 * When a field is formatted for display, the display formatter does not know
 * what view mode it is being displayed for. Unfortunately, the Add to Cart form
 * display formatter needs this information when displaying product reference
 * fields on nodes to provide adequate context for product field replacement on
 * multi-value product reference fields. This hook is used to transform a set of
 * arguments into a form using the arguments and the extra context information
 * gleaned from the parameters passed into this function.
 */
function commerce_express_checkout_field_attach_view_alter(&$output, $context) {
  // Loop through the fields passed in looking for any product reference fields
  // formatted with the Add to Cart form display formatter.
  foreach ($output as $field_name => $element) {
    if (!empty($element['#formatter']) && $element['#formatter'] == 'commerce_express_checkout_select') {
      // Prepare the context information needed by the cart form.
      $checkout_context = $context;

      // Remove the full entity from the context array and put the ID in instead.
      list($entity_id, $vid, $bundle) = entity_extract_ids($context['entity_type'], $context['entity']);
      $checkout_context['entity_id'] = $entity_id;
      $checkout_context['bundle'] = $bundle;
      unset($checkout_context['entity']);

      // Remove any Views data added to the context by views_handler_field_field.
      // It unnecessarily increases the size of rows in the cache_form table for
      // Add to Cart form state data.
      if (!empty($checkout_context['display']) && is_array($checkout_context['display'])) {
        unset($checkout_context['display']['views_view']);
        unset($checkout_context['display']['views_field']);
        unset($checkout_context['display']['views_row_id']);
      }

      // Add the context for displaying product fields in the context of an entity
      // that references the product by looking at the entity this product
      // reference field is attached to.
      $checkout_context['class_prefix'] = $context['entity_type'] . '-' . $entity_id;
      $checkout_context['view_mode'] = $context['entity_type'] . '_' . $element['#view_mode'];

      $entity_uri = entity_uri($context['entity_type'], $element['#object']);

      foreach (element_children($element) as $key) {
        // Extract the drupal_get_form() arguments array from the element.
        $arguments = $element[$key]['#arguments'];

        // Replace the array containing the arguments with the return value of
        // drupal_get_form(). It will be rendered when the rest of the object is
        // rendered for display.
        $output[$field_name][$key] = drupal_get_form($arguments['form_id'], $arguments['products'], $arguments['line_item_type'], $arguments['display'], $checkout_context);
      }
    }
  }
}

/**
 * Define the express checkout form.
 *
 * @param $form
 * @param $form_state
 * @param $products
 * @param $line_item_type
 * @param $display
 *  The field formatter display value from formatter_settings_form.
 * @param $checkout_context
 */
function commerce_express_checkout_express_checkout_form($form, &$form_state, $products, $line_item_type, $display, $checkout_context) {

  $options = array();

  foreach($products as $product_id => $product) {
    $options[$product_id] = $product->title;
  }

  if (isset($form_state['values']['product_select']) &&
    !empty($form_state['values']['product_select'])) {
    $default_product_id = $form_state['values']['product_select'];
  }
  else {
    $product_ids = array_keys($options);
    $default_product_id = reset($product_ids);
  }

  $form_state['default_product_id'] = $default_product_id;
  $form_state['line_item_type'] = $line_item_type;
  $form_state['context'] = $checkout_context;

  // Store the form ID as a class of the form to avoid the incrementing form ID
  // from causing the AJAX refresh not to work.
  $form['#attributes']['class'][] = drupal_html_class('commerce_express_checkout_express_checkout_form');

  $form['product_select'] = array(
    '#type' => 'radios',
    '#title' => t('Products'),
    '#options' => $options,
    '#default_value' => $default_product_id,
    '#ajax' => array(
      'callback' => 'commerce_express_checkout_form_attributes_refresh',
    ),
  );

  $settings = $display['settings'];

  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => !empty($settings['text_to_display']) ? $settings['text_to_display'] : t('Express checkout'),
  );

  $form['#submit'][] = 'commerce_express_checkout_express_checkout_form_submit';

  return $form;
}

/**
 * Submit handler.
 */
function commerce_express_checkout_express_checkout_form_submit($form, &$form_state) {

  $options = array(
    'entity_id' => $form_state['context']['entity_id'],
    'entity_type' => $form_state['context']['entity_type'],
    'bundle' => $form_state['context']['bundle'],
    'quantity' => 1,
    'line_item_type' => $form_state['line_item_type'],
  );

  $url = commerce_express_checkout_generate_link($form_state['default_product_id']);

  drupal_goto($url, array('query' => $options));
}

/**
 * Copy from commerce_cart_add_to_cart_form_attributes_refresh()
 */
function commerce_express_checkout_form_attributes_refresh($form, $form_state) {
  return commerce_cart_add_to_cart_form_attributes_refresh($form, $form_state);
}
