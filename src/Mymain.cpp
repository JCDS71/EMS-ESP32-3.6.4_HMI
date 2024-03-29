/*
 * EMS-ESP - https://github.com/emsesp/EMS-ESP
 * Copyright 2020-2023  Paul Derbyshire
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emsesp.h"
#include "common_Display.h"
#include <uuid/log.h>

//char *Hc1_Mode = "auto";

static emsesp::EMSESP application;

void setup() {
    application.start();
    displayInit();
}

void loop() {
    lv_timer_handler(); // let the GUI do its work 
    delay(5);
    //Serial.print("my time is : ");
   //Serial.println(uuid::log::format_timestamp_ms(123456789UL, 3).c_str());
    application.loop();
    //std::string msgstr = uuid::log::format_timestamp_ms(message.content_->uptime_ms, 3)
    //uuid::log::PrintHandler::print_.print(uuid::log::format_timestamp_ms(uuid::log::Message->uptime_ms, 3));
}
