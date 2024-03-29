import { useContext, useEffect } from 'react';
import { Navigate, useLocation } from 'react-router-dom';

import type { AuthenticatedContextValue } from 'contexts/authentication/context';
import type { FC } from 'react';

import type { RequiredChildrenProps } from 'utils';
import { storeLoginRedirect } from 'api/authentication';
import { AuthenticatedContext, AuthenticationContext } from 'contexts/authentication/context';

const RequireAuthenticated: FC<RequiredChildrenProps> = ({ children }) => {
  const authenticationContext = useContext(AuthenticationContext);
  const location = useLocation();

  useEffect(() => {
    if (!authenticationContext.me) {
      storeLoginRedirect(location);
    }
  });

  return authenticationContext.me ? (
    <AuthenticatedContext.Provider value={authenticationContext as AuthenticatedContextValue}>
      {children}
    </AuthenticatedContext.Provider>
  ) : (
    <Navigate to="/unauthorized" />
  );
};

export default RequireAuthenticated;
