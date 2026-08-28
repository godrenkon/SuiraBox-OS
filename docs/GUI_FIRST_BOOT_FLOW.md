# SuiraBox GUI First-Boot Flow

## Final Desktop behavior

The Desktop edition must never require a terminal or serial console for normal first boot.

The intended flow is:

1. Firmware/bootloader starts SuiraBox.
2. Kernel initializes the minimum required services.
3. Desktop compositor/window system starts.
4. A compact modal dialog appears above the desktop.
5. The user opens a single language select box and chooses one option from its dropdown list.
6. The user presses `Continue`.
7. The choice is validated and atomically persisted.
8. The dialog closes and the normal desktop becomes interactive.

## Language selector

The language control is a standard select-box interaction, not a set of independent language buttons.

Initial choices:

- Japanese
- English
- Chinese
- Spanish

The selector has one closed field showing the current value and one expandable option list. Keyboard navigation uses `Space` to open/close, `Up`/`Down` to change the highlighted option, and `Enter` to accept the highlighted option. The closed selector can then be confirmed to continue.

The pre-selection UI may use English because the user must be able to understand the selector before choosing a language.

## Persistence requirements

The configuration record must include at least:

- format version
- selected language
- first-boot-completed flag
- integrity/check value

Writes must be atomic. A failed or interrupted write must leave either the previous valid record or a recoverable incomplete record; it must never leave an apparently valid but partially written configuration.

If the file is missing, malformed, unsupported, or fails integrity validation, the system treats the machine as first-boot and shows the language selector again.

## Reboot behavior

After successful persistence:

`Boot -> Desktop -> no first-boot dialog`

The localization service loads the saved language before the Settings, notification, error, installer, Store, and application UI become visible.

## Separation from CUI

This flow is specific to the Desktop edition. The future Server/CUI edition will have its own text-mode setup and must not add terminal requirements to Desktop.

## Performance rule

The first-boot dialog must not require Chrome, package-manager services, cloud services, optional drivers, or other optional applications. It should depend only on the minimum GUI and configuration services needed to render the dialog and save the result.
