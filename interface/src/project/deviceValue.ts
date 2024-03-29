import { DeviceValueUOM, DeviceValueUOM_s } from './types';
import type { TranslationFunctions } from 'i18n/i18n-types';

const formatDurationMin = (LL: TranslationFunctions, duration_min: number) => {
  const days = Math.trunc((duration_min * 60000) / 86400000);
  const hours = Math.trunc((duration_min * 60000) / 3600000) % 24;
  const minutes = Math.trunc((duration_min * 60000) / 60000) % 60;

  let formatted = '';
  if (days) {
    formatted += LL.NUM_DAYS({ num: days });
  }

  if (hours) {
    if (formatted) formatted += ' ';
    formatted += LL.NUM_HOURS({ num: hours });
  }

  if (minutes) {
    if (formatted) formatted += ' ';
    formatted += LL.NUM_MINUTES({ num: minutes });
  }

  return formatted;
};

export function formatValue(LL: TranslationFunctions, value: any, uom: number) {
  if (value === undefined) {
    return '';
  }
  switch (uom) {
    case DeviceValueUOM.HOURS:
      return value ? formatDurationMin(LL, value * 60) : LL.NUM_HOURS({ num: 0 });
    case DeviceValueUOM.MINUTES:
      return value ? formatDurationMin(LL, value) : LL.NUM_MINUTES({ num: 0 });
    case DeviceValueUOM.SECONDS:
      return LL.NUM_SECONDS({ num: value });
    case DeviceValueUOM.NONE:
      if (typeof value === 'number') {
        return new Intl.NumberFormat().format(value);
      }
      return value;
    case DeviceValueUOM.DEGREES:
    case DeviceValueUOM.DEGREES_R:
    case DeviceValueUOM.FAHRENHEIT:
      return (
        new Intl.NumberFormat(undefined, {
          minimumFractionDigits: 1
        }).format(value) +
        ' ' +
        DeviceValueUOM_s[uom]
      );
    default:
      return new Intl.NumberFormat().format(value) + ' ' + DeviceValueUOM_s[uom];
  }
}
