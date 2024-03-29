import { Tab } from '@mui/material';
import { useContext } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import NTPSettingsForm from './NTPSettingsForm';
import NTPStatusForm from './NTPStatusForm';
import type { FC } from 'react';

import { RequireAdmin, RouterTabs, useLayoutTitle, useRouterTab } from 'components';
import { AuthenticatedContext } from 'contexts/authentication';

import { useI18nContext } from 'i18n/i18n-react';

const NetworkTime: FC = () => {
  const { LL } = useI18nContext();
  useLayoutTitle('NTP');

  const authenticatedContext = useContext(AuthenticatedContext);
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="/ntp/status" label={LL.STATUS_OF('NTP')} />
        <Tab value="/ntp/settings" label={LL.SETTINGS_OF('NTP')} disabled={!authenticatedContext.me.admin} />
      </RouterTabs>
      <Routes>
        <Route path="status" element={<NTPStatusForm />} />
        <Route
          path="settings"
          element={
            <RequireAdmin>
              <NTPSettingsForm />
            </RequireAdmin>
          }
        />
        <Route path="*" element={<Navigate replace to="/ntp/status" />} />
      </Routes>
    </>
  );
};

export default NetworkTime;
