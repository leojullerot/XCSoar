/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Device/Driver/XCOM760.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"

#include <stdio.h>

class XCOM760Device : public AbstractDevice {
private:
  Port *port;

public:
  XCOM760Device(Port *_port):port(_port) {}

public:
  virtual bool PutVolume(int volume);
  virtual bool PutActiveFrequency(RadioFrequency frequency);
  virtual bool PutStandbyFrequency(RadioFrequency frequency);
};

bool
XCOM760Device::PutVolume(int Volume)
{
  char szTmp[32];
  sprintf(szTmp, "$RVOL=%d\r\n", Volume);
  port->Write(szTmp);
  return true;
}

bool
XCOM760Device::PutActiveFrequency(RadioFrequency frequency)
{
  char szTmp[32];
  sprintf(szTmp, "$TXAF=%u.%03u\r\n",
          frequency.GetKiloHertz() / 1000,
          frequency.GetKiloHertz() % 1000);
  port->Write(szTmp);
  return true;
}

bool
XCOM760Device::PutStandbyFrequency(RadioFrequency frequency)
{
  char szTmp[32];
  sprintf(szTmp, "$TXSF=%u.%03u\r\n",
          frequency.GetKiloHertz() / 1000,
          frequency.GetKiloHertz() % 1000);
  return true;
}

static Device *
XCOM760CreateOnPort(const DeviceConfig &config, Port *com_port)
{
  return new XCOM760Device(com_port);
}

const struct DeviceRegister xcom760Device = {
  _T("XCOM760"),
  _T("XCOM760"),
  0,
  XCOM760CreateOnPort,
};

/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
