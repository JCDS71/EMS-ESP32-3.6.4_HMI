import CancelIcon from '@mui/icons-material/Cancel';
import CloseIcon from '@mui/icons-material/Close';
import DoneIcon from '@mui/icons-material/Done';

import {
  Box,
  Button,
  Dialog,
  DialogActions,
  DialogContent,
  DialogTitle,
  Grid,
  TextField,
  Typography
} from '@mui/material';
import { useEffect, useState } from 'react';

import EntityMaskToggle from './EntityMaskToggle';
import { DeviceEntityMask } from './types';
import type { DeviceEntity } from './types';

import { dialogStyle } from 'CustomTheme';
import { useI18nContext } from 'i18n/i18n-react';

import { updateValue } from 'utils';

type SettingsCustomizationDialogProps = {
  open: boolean;
  onClose: () => void;
  onSave: (di: DeviceEntity) => void;
  selectedItem: DeviceEntity;
};

const SettingsCustomizationDialog = ({ open, onClose, onSave, selectedItem }: SettingsCustomizationDialogProps) => {
  const { LL } = useI18nContext();
  const [editItem, setEditItem] = useState<DeviceEntity>(selectedItem);
  const [error, setError] = useState<boolean>(false);

  const updateFormValue = updateValue(setEditItem);

  const isWriteableNumber =
    typeof editItem.v === 'number' && editItem.w && !(editItem.m & DeviceEntityMask.DV_READONLY);

  useEffect(() => {
    if (open) {
      setError(false);
      setEditItem(selectedItem);
    }
  }, [open, selectedItem]);

  const close = () => {
    onClose();
  };

  const save = () => {
    if (isWriteableNumber && editItem.mi && editItem.ma && editItem.mi > editItem?.ma) {
      setError(true);
    } else {
      onSave(editItem);
    }
  };

  const updateDeviceEntity = (updatedItem: DeviceEntity) => {
    setEditItem({ ...editItem, m: updatedItem.m });
  };

  return (
    <Dialog sx={dialogStyle} open={open} onClose={close}>
      <DialogTitle>{LL.EDIT() + ' ' + LL.ENTITY()}</DialogTitle>
      <DialogContent dividers>
        <Grid container direction="row">
          <Typography variant="body2" color="warning.main">
            {LL.ENTITY() + ' ID'}:&nbsp;
          </Typography>
          <Typography variant="body2">{editItem.id}</Typography>
        </Grid>

        <Grid container direction="row">
          <Typography variant="body2" color="warning.main">
            {LL.DEFAULT(1) + ' ' + LL.ENTITY_NAME(1)}:&nbsp;
          </Typography>
          <Typography variant="body2">{editItem.n}</Typography>
        </Grid>

        <Grid container direction="row">
          <Typography variant="body2" color="warning.main">
            {LL.WRITEABLE()}:&nbsp;
          </Typography>
          <Typography variant="body2">
            {editItem.w ? (
              <DoneIcon color="success" sx={{ fontSize: 16 }} />
            ) : (
              <CloseIcon color="error" sx={{ fontSize: 16 }} />
            )}
          </Typography>
        </Grid>

        <Box mt={1} mb={2}>
          <EntityMaskToggle onUpdate={updateDeviceEntity} de={editItem} />
        </Box>
        <Grid container spacing={1}>
          <Grid item>
            <TextField
              name="cn"
              label={LL.NEW_NAME_OF(LL.ENTITY())}
              value={editItem.cn}
              autoFocus
              sx={{ width: '30ch' }}
              onChange={updateFormValue}
            />
          </Grid>
          {isWriteableNumber && (
            <>
              <Grid item>
                <TextField
                  name="mi"
                  label={LL.MIN()}
                  value={editItem.mi}
                  sx={{ width: '8ch' }}
                  type="number"
                  onChange={updateFormValue}
                />
              </Grid>
              <Grid item>
                <TextField
                  name="ma"
                  label={LL.MAX()}
                  value={editItem.ma}
                  sx={{ width: '8ch' }}
                  type="number"
                  onChange={updateFormValue}
                />
              </Grid>
            </>
          )}
        </Grid>
        {error && (
          <Typography variant="body2" color="error" mt={2}>
            Error: Check min and max values
          </Typography>
        )}
      </DialogContent>
      <DialogActions>
        <Button startIcon={<CancelIcon />} variant="outlined" onClick={close} color="secondary">
          {LL.CANCEL()}
        </Button>
        <Button startIcon={<DoneIcon />} variant="outlined" onClick={save} color="primary">
          {LL.UPDATE()}
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default SettingsCustomizationDialog;
