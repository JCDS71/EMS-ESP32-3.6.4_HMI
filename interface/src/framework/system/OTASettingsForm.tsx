import CancelIcon from '@mui/icons-material/Cancel';
import WarningIcon from '@mui/icons-material/Warning';
import { Button, Checkbox } from '@mui/material';
import { useState } from 'react';
import type { ValidateFieldsError } from 'async-validator';
import type { FC } from 'react';

import type { OTASettings } from 'types';
import * as SystemApi from 'api/system';
import {
  BlockFormControlLabel,
  ButtonRow,
  FormLoader,
  SectionContent,
  ValidatedPasswordField,
  ValidatedTextField,
  BlockNavigation
} from 'components';

import { useI18nContext } from 'i18n/i18n-react';
import { numberValue, updateValueDirty, useRest } from 'utils';

import { validate } from 'validators';
import { OTA_SETTINGS_VALIDATOR } from 'validators/system';

const OTASettingsForm: FC = () => {
  const {
    loadData,
    saveData,
    saving,
    updateDataValue,
    data,
    origData,
    dirtyFlags,
    setDirtyFlags,
    blocker,
    errorMessage
  } = useRest<OTASettings>({
    read: SystemApi.readOTASettings,
    update: SystemApi.updateOTASettings
  });

  const { LL } = useI18nContext();

  const updateFormValue = updateValueDirty(origData, dirtyFlags, setDirtyFlags, updateDataValue);

  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();

  const content = () => {
    if (!data) {
      return <FormLoader onRetry={loadData} errorMessage={errorMessage} />;
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(OTA_SETTINGS_VALIDATOR, data);
        await saveData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    return (
      <>
        <BlockFormControlLabel
          control={<Checkbox name="enabled" checked={data.enabled} onChange={updateFormValue} />}
          label={LL.ENABLE_OTA()}
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="port"
          label="Port"
          fullWidth
          variant="outlined"
          value={numberValue(data.port)}
          type="number"
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedPasswordField
          fieldErrors={fieldErrors}
          name="password"
          label={LL.PASSWORD()}
          fullWidth
          variant="outlined"
          value={data.password}
          onChange={updateFormValue}
          margin="normal"
        />
        {dirtyFlags && dirtyFlags.length !== 0 && (
          <ButtonRow>
            <Button
              startIcon={<CancelIcon />}
              disabled={saving}
              variant="outlined"
              color="primary"
              type="submit"
              onClick={loadData}
            >
              {LL.CANCEL()}
            </Button>
            <Button
              startIcon={<WarningIcon color="warning" />}
              disabled={saving}
              variant="contained"
              color="info"
              type="submit"
              onClick={validateAndSubmit}
            >
              {LL.APPLY_CHANGES(dirtyFlags.length)}
            </Button>
          </ButtonRow>
        )}
      </>
    );
  };

  return (
    <SectionContent title={LL.SETTINGS_OF('OTA')} titleGutter>
      {blocker ? <BlockNavigation blocker={blocker} /> : null}
      {content()}
    </SectionContent>
  );
};

export default OTASettingsForm;
