<?php


/**
 * @file
 * Content type that displays the relcontext context type.
 *
 * This example is for use with the relcontext relationship to show
 * how we can get a relationship-context into a data type.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Custom link'),
  'description' => t('Displays a configurable link, that leads to some other page.'),
  'single' => TRUE,
  'render callback' => 'ctools_custom_link_render',
  'category' => t('Custom'),
  'top level' => TRUE,
  'edit form' => 'ctools_custom_link_edit_form',
  'all contexts' => TRUE,
  'admin title' => 'ctools_custom_link_admin_title',
  'admin info' => 'ctools_custom_link_admin_info',
);

/**
 * Run-time rendering of the body of the block.
 *
 * @param $subtype
 * @param $conf
 * @param $args
 * @param $contexts
 *
 * @return \stdClass An object with at least title and content members
 */
function ctools_custom_link_render($subtype, $conf, $args, $contexts) {
  $block = new stdClass();

  // Make sure url and text is set, if was called through ctools_content_render.
  $url = !isset($conf['url']) ? '<front>' : $conf['url'];
  $text = !isset($conf['text']) ? 'Link text was not set' : $conf['text'];

  // First replace tokens.
  $url = token_replace($url);
  $text = token_replace($text);

  // Then replace keyword substitutions if we were configured to do so.
  if (!empty($contexts) && !empty($conf['substitute'])) {
    $url = ctools_custom_link_apply_substitution($url, $contexts);
    $text = ctools_custom_link_apply_substitution($text, $contexts);
  }

  // Default options, so theme_link doesn't yell undefined index.
  $options = array(
    'attributes' => array(),
    'html' => FALSE,
  );

  // Run each class through token_replace, keyword substitutions, and add them to attributes.
  if (isset($conf['classes']) && !empty($conf['classes'])) {
    $classes = explode(' ', $conf['classes']);
    $classes = array_map('token_replace', $classes);

    if (!empty($contexts) && !empty($conf['substitute'])) {
      foreach ($classes as &$class) {
        $class = ctools_custom_link_apply_substitution($class, $contexts);
      }
    }
    $options['attributes']['class'] = $classes;
  }

  // Check if the text should not be escaped.
  if (isset($conf['html']) && !empty($conf['html'])) {
    $options['html'] = TRUE;
  }

  // Extract the attributes from the text-area and add them to the attributes array.
  if (!empty($conf['attributes'])) {
    $attributes = ctools_custom_link_extract_allowed_values($conf['attributes'], 'list_text', FALSE);
    if (!empty($attributes)) {
      $options['attributes'] += $attributes;
    }
  }

  // Render the link.
  $link = l(t($text), $url, $options);

  $block->title = '';
  $block->content = $link;

  return $block;
}

/**
 * Replaces substitutions with values.
 *
 * @param $contexts
 * @param $content
 * @return string
 */
function ctools_custom_link_apply_substitution($content, $contexts) {
  $content = ctools_context_keyword_substitute($content, array(), $contexts);
  return $content;
}

/**
 * Returns an edit form to customize the block content.
 */
function ctools_custom_link_edit_form($form, &$form_state) {
  $conf = $form_state['conf'];

  $url = isset($conf['url']) ? $conf['url'] : '';
  $text = isset($conf['text']) ? $conf['text'] : '';
  $classes = isset($conf['classes']) ? $conf['classes'] : '';
  $html = isset($conf['html']) ? $conf['html'] : 0;
  $attributes = isset($conf['attributes']) ? $conf['attributes'] : '';
  $substitute = isset($conf['substitute']) ? $conf['substitute'] : '';

  $form['url'] = array(
    '#title' => t('URL'),
    '#description' => t('Where the link should lead. This can be an internal Drupal path such as node/add or an external URL such as http://drupal.org. Enter <front> to link to the front page. This field supports tokens.'),
    '#type' => 'textfield',
    '#default_value' => $url,
    '#required' => TRUE,
  );

  // Display token list.
  if (module_exists('token')) {
    $form['token_tree'] = array(
      '#theme' => 'token_tree',
      '#dialog' => TRUE,
    );
  }

  $form['text'] = array(
    '#title' => t('Text'),
    '#description' => t('The link text. This field supports tokens.'),
    '#type' => 'textfield',
    '#default_value' => $text,
    '#required' => TRUE,
  );

  $form['classes'] = array(
    '#title' => t('CSS classes'),
    '#description' => t('Classes should be delimited with spaces. This field supports tokens.'),
    '#type' => 'textfield',
    '#default_value' => $classes,
  );

  $form['html'] = array(
    '#title' => t('Text contains html'),
    '#description' => t('Check this if the link text contains html code, so it does not get escaped.'),
    '#type' => 'checkbox',
    '#default_value' => $html,
  );

  $form['attributes'] = array(
    '#title' => t('Additional HTML attributes'),
    '#description' => t('Each attribute will be added to the attributes array of the l() function.
Enter one value per line, in the format attribute|value.'),
    '#type' => 'textarea',
    '#default_value' => $attributes,
  );

  // Add context substitutions.
  if (!empty($form_state['contexts'])) {
    // Set extended description if both CCK and Token modules are enabled, notifying of unlisted keywords
    if (module_exists('content') && module_exists('token')) {
      $description = t('If checked, context keywords will be substituted in the url, text, and classes. Note that CCK fields may be used as keywords using patterns like <em>%node:field_name-formatted</em>.');
    }
    elseif (!module_exists('token')) {
      $description = t('If checked, context keywords will be substituted in the url, text, and classes. More keywords will be available if you install the Token module, see http://drupal.org/project/token.');
    }
    else {
      $description = t('If checked, context keywords will be substituted in the url, text, and classes.');
    }

    $form['substitute'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use context keywords'),
      '#description' => $description,
      '#default_value' => $substitute,
    );
    $form['contexts'] = array(
      '#title' => t('Substitutions'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $rows = array();
    foreach ($form_state['contexts'] as $context) {
      foreach (ctools_context_get_converters('%' . check_plain($context->keyword) . ':', $context) as $keyword => $title) {
        $rows[] = array(
          check_plain($keyword),
          t('@identifier: @title', array('@title' => $title, '@identifier' => $context->identifier)),
        );
      }
    }
    $header = array(t('Keyword'), t('Value'));
    $form['contexts']['context'] = array('#markup' => theme('table', array('header' => $header, 'rows' => $rows)));
  }

  return $form;
}

/**
 * Save the settings.
 */
function ctools_custom_link_edit_form_submit($form, &$form_state) {
  $form_state['conf']['url'] = $form_state['values']['url'];
  $form_state['conf']['text'] = $form_state['values']['text'];
  $form_state['conf']['classes'] = $form_state['values']['classes'];
  $form_state['conf']['html'] = $form_state['values']['html'];
  $form_state['conf']['attributes'] = $form_state['values']['attributes'];
  $form_state['conf']['substitute'] = $form_state['values']['substitute'];
}

/**
 * Displays the admin title in the pane top bar.
 *
 * @param $subtype
 * @param $conf
 * @param null $context
 * @return string
 */
function ctools_custom_link_admin_title($subtype, $conf, $context = NULL) {
  $output = t('Custom link: ') . check_plain($conf['text']);
  return $output;
}

/**
 * Displays a preview of the content, when arrow is clicked on the panel pane.
 *
 * @param $subtype
 * @param $conf
 * @param null $context
 * @return stdClass
 */
function ctools_custom_link_admin_info($subtype, $conf, $context = NULL) {
  $block = ctools_custom_link_render($subtype, $conf, array(), $context);
  return $block;
}


/**
 * Parses a string of 'allowed values' into an array.
 *
 * Copy-pasted from list.module, so we don't depend on it.
 *
 * @param $string
 *   The list of allowed values in string format described in
 *   list_allowed_values_string().
 * @param $field_type
 *   The field type. Either 'list_number' or 'list_text'.
 * @param $generate_keys
 *   Boolean value indicating whether to generate keys based on the position of
 *   the value if a key is not manually specified, and if the value cannot be
 *   used as a key. This should only be TRUE for fields of type 'list_number'.
 *
 * @return array The array of extracted key/value pairs, or NULL if the string is invalid.
 */
function ctools_custom_link_extract_allowed_values($string, $field_type, $generate_keys) {
  $values = array();

  $list = explode("\n", $string);
  $list = array_map('trim', $list);
  $list = array_filter($list, 'strlen');

  $generated_keys = $explicit_keys = FALSE;
  foreach ($list as $position => $text) {
    $value = $key = FALSE;

    // Check for an explicit key.
    $matches = array();
    if (preg_match('/(.*)\|(.*)/', $text, $matches)) {
      $key = $matches[1];
      $value = $matches[2];
      $explicit_keys = TRUE;
    }
    // Otherwise see if we can use the value as the key. Detecting true integer
    // strings takes a little trick.
    elseif ($field_type == 'list_text'
      || ($field_type == 'list_float' && is_numeric($text))
      || ($field_type == 'list_integer' && is_numeric($text) && (float) $text == intval($text))) {
      $key = $value = $text;
      $explicit_keys = TRUE;
    }
    // Otherwise see if we can generate a key from the position.
    elseif ($generate_keys) {
      $key = (string) $position;
      $value = $text;
      $generated_keys = TRUE;
    }
    else {
      return;
    }

    // Float keys are represented as strings and need to be disambiguated
    // ('.5' is '0.5').
    if ($field_type == 'list_float' && is_numeric($key)) {
      $key = (string) (float) $key;
    }

    $values[$key] = $value;
  }

  // We generate keys only if the list contains no explicit key at all.
  if ($explicit_keys && $generated_keys) {
    return;
  }

  return $values;
}