// MIT License

// Copyright (c) 2017 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nekit/utils/country_iso_code.h"

namespace nekit {
namespace utils {
CountryIsoCode CountryIsoCodeFromString(std::string code) {
  if (code == "CN")
    return CountryIsoCode::CN;
  else if (code == "HK")
    return CountryIsoCode::HK;
  else if (code == "TW")
    return CountryIsoCode::TW;
  else if (code == "US")
    return CountryIsoCode::US;
  else if (code == "JP")
    return CountryIsoCode::JP;
  else if (code == "KR")
    return CountryIsoCode::KR;
  else if (code == "GB")
    return CountryIsoCode::GB;
  else if (code == "AF")
    return CountryIsoCode::AF;
  else if (code == "AX")
    return CountryIsoCode::AX;
  else if (code == "AL")
    return CountryIsoCode::AL;
  else if (code == "DZ")
    return CountryIsoCode::DZ;
  else if (code == "AS")
    return CountryIsoCode::AS;
  else if (code == "AD")
    return CountryIsoCode::AD;
  else if (code == "AO")
    return CountryIsoCode::AO;
  else if (code == "AI")
    return CountryIsoCode::AI;
  else if (code == "AQ")
    return CountryIsoCode::AQ;
  else if (code == "AG")
    return CountryIsoCode::AG;
  else if (code == "AR")
    return CountryIsoCode::AR;
  else if (code == "AM")
    return CountryIsoCode::AM;
  else if (code == "AW")
    return CountryIsoCode::AW;
  else if (code == "AU")
    return CountryIsoCode::AU;
  else if (code == "AT")
    return CountryIsoCode::AT;
  else if (code == "AZ")
    return CountryIsoCode::AZ;
  else if (code == "BS")
    return CountryIsoCode::BS;
  else if (code == "BH")
    return CountryIsoCode::BH;
  else if (code == "BD")
    return CountryIsoCode::BD;
  else if (code == "BB")
    return CountryIsoCode::BB;
  else if (code == "BY")
    return CountryIsoCode::BY;
  else if (code == "BE")
    return CountryIsoCode::BE;
  else if (code == "BZ")
    return CountryIsoCode::BZ;
  else if (code == "BJ")
    return CountryIsoCode::BJ;
  else if (code == "BM")
    return CountryIsoCode::BM;
  else if (code == "BT")
    return CountryIsoCode::BT;
  else if (code == "BO")
    return CountryIsoCode::BO;
  else if (code == "BQ")
    return CountryIsoCode::BQ;
  else if (code == "BA")
    return CountryIsoCode::BA;
  else if (code == "BW")
    return CountryIsoCode::BW;
  else if (code == "BV")
    return CountryIsoCode::BV;
  else if (code == "BR")
    return CountryIsoCode::BR;
  else if (code == "IO")
    return CountryIsoCode::IO;
  else if (code == "BN")
    return CountryIsoCode::BN;
  else if (code == "BG")
    return CountryIsoCode::BG;
  else if (code == "BF")
    return CountryIsoCode::BF;
  else if (code == "BI")
    return CountryIsoCode::BI;
  else if (code == "CV")
    return CountryIsoCode::CV;
  else if (code == "KH")
    return CountryIsoCode::KH;
  else if (code == "CM")
    return CountryIsoCode::CM;
  else if (code == "CA")
    return CountryIsoCode::CA;
  else if (code == "KY")
    return CountryIsoCode::KY;
  else if (code == "CF")
    return CountryIsoCode::CF;
  else if (code == "TD")
    return CountryIsoCode::TD;
  else if (code == "CL")
    return CountryIsoCode::CL;
  else if (code == "CX")
    return CountryIsoCode::CX;
  else if (code == "CC")
    return CountryIsoCode::CC;
  else if (code == "CO")
    return CountryIsoCode::CO;
  else if (code == "KM")
    return CountryIsoCode::KM;
  else if (code == "CG")
    return CountryIsoCode::CG;
  else if (code == "CD")
    return CountryIsoCode::CD;
  else if (code == "CK")
    return CountryIsoCode::CK;
  else if (code == "CR")
    return CountryIsoCode::CR;
  else if (code == "CI")
    return CountryIsoCode::CI;
  else if (code == "HR")
    return CountryIsoCode::HR;
  else if (code == "CU")
    return CountryIsoCode::CU;
  else if (code == "CW")
    return CountryIsoCode::CW;
  else if (code == "CY")
    return CountryIsoCode::CY;
  else if (code == "CZ")
    return CountryIsoCode::CZ;
  else if (code == "DK")
    return CountryIsoCode::DK;
  else if (code == "DJ")
    return CountryIsoCode::DJ;
  else if (code == "DM")
    return CountryIsoCode::DM;
  else if (code == "DO")
    return CountryIsoCode::DO;
  else if (code == "EC")
    return CountryIsoCode::EC;
  else if (code == "EG")
    return CountryIsoCode::EG;
  else if (code == "SV")
    return CountryIsoCode::SV;
  else if (code == "GQ")
    return CountryIsoCode::GQ;
  else if (code == "ER")
    return CountryIsoCode::ER;
  else if (code == "EE")
    return CountryIsoCode::EE;
  else if (code == "ET")
    return CountryIsoCode::ET;
  else if (code == "FK")
    return CountryIsoCode::FK;
  else if (code == "FO")
    return CountryIsoCode::FO;
  else if (code == "FJ")
    return CountryIsoCode::FJ;
  else if (code == "FI")
    return CountryIsoCode::FI;
  else if (code == "FR")
    return CountryIsoCode::FR;
  else if (code == "GF")
    return CountryIsoCode::GF;
  else if (code == "PF")
    return CountryIsoCode::PF;
  else if (code == "TF")
    return CountryIsoCode::TF;
  else if (code == "GA")
    return CountryIsoCode::GA;
  else if (code == "GM")
    return CountryIsoCode::GM;
  else if (code == "GE")
    return CountryIsoCode::GE;
  else if (code == "DE")
    return CountryIsoCode::DE;
  else if (code == "GH")
    return CountryIsoCode::GH;
  else if (code == "GI")
    return CountryIsoCode::GI;
  else if (code == "GR")
    return CountryIsoCode::GR;
  else if (code == "GL")
    return CountryIsoCode::GL;
  else if (code == "GD")
    return CountryIsoCode::GD;
  else if (code == "GP")
    return CountryIsoCode::GP;
  else if (code == "GU")
    return CountryIsoCode::GU;
  else if (code == "GT")
    return CountryIsoCode::GT;
  else if (code == "GG")
    return CountryIsoCode::GG;
  else if (code == "GN")
    return CountryIsoCode::GN;
  else if (code == "GW")
    return CountryIsoCode::GW;
  else if (code == "GY")
    return CountryIsoCode::GY;
  else if (code == "HT")
    return CountryIsoCode::HT;
  else if (code == "HM")
    return CountryIsoCode::HM;
  else if (code == "VA")
    return CountryIsoCode::VA;
  else if (code == "HN")
    return CountryIsoCode::HN;
  else if (code == "HU")
    return CountryIsoCode::HU;
  else if (code == "IS")
    return CountryIsoCode::IS;
  else if (code == "IN")
    return CountryIsoCode::IN;
  else if (code == "ID")
    return CountryIsoCode::ID;
  else if (code == "IR")
    return CountryIsoCode::IR;
  else if (code == "IQ")
    return CountryIsoCode::IQ;
  else if (code == "IE")
    return CountryIsoCode::IE;
  else if (code == "IM")
    return CountryIsoCode::IM;
  else if (code == "IL")
    return CountryIsoCode::IL;
  else if (code == "IT")
    return CountryIsoCode::IT;
  else if (code == "JM")
    return CountryIsoCode::JM;
  else if (code == "JE")
    return CountryIsoCode::JE;
  else if (code == "JO")
    return CountryIsoCode::JO;
  else if (code == "KZ")
    return CountryIsoCode::KZ;
  else if (code == "KE")
    return CountryIsoCode::KE;
  else if (code == "KI")
    return CountryIsoCode::KI;
  else if (code == "KP")
    return CountryIsoCode::KP;
  else if (code == "KW")
    return CountryIsoCode::KW;
  else if (code == "KG")
    return CountryIsoCode::KG;
  else if (code == "LA")
    return CountryIsoCode::LA;
  else if (code == "LV")
    return CountryIsoCode::LV;
  else if (code == "LB")
    return CountryIsoCode::LB;
  else if (code == "LS")
    return CountryIsoCode::LS;
  else if (code == "LR")
    return CountryIsoCode::LR;
  else if (code == "LY")
    return CountryIsoCode::LY;
  else if (code == "LI")
    return CountryIsoCode::LI;
  else if (code == "LT")
    return CountryIsoCode::LT;
  else if (code == "LU")
    return CountryIsoCode::LU;
  else if (code == "MO")
    return CountryIsoCode::MO;
  else if (code == "MK")
    return CountryIsoCode::MK;
  else if (code == "MG")
    return CountryIsoCode::MG;
  else if (code == "MW")
    return CountryIsoCode::MW;
  else if (code == "MY")
    return CountryIsoCode::MY;
  else if (code == "MV")
    return CountryIsoCode::MV;
  else if (code == "ML")
    return CountryIsoCode::ML;
  else if (code == "MT")
    return CountryIsoCode::MT;
  else if (code == "MH")
    return CountryIsoCode::MH;
  else if (code == "MQ")
    return CountryIsoCode::MQ;
  else if (code == "MR")
    return CountryIsoCode::MR;
  else if (code == "MU")
    return CountryIsoCode::MU;
  else if (code == "YT")
    return CountryIsoCode::YT;
  else if (code == "MX")
    return CountryIsoCode::MX;
  else if (code == "FM")
    return CountryIsoCode::FM;
  else if (code == "MD")
    return CountryIsoCode::MD;
  else if (code == "MC")
    return CountryIsoCode::MC;
  else if (code == "MN")
    return CountryIsoCode::MN;
  else if (code == "ME")
    return CountryIsoCode::ME;
  else if (code == "MS")
    return CountryIsoCode::MS;
  else if (code == "MA")
    return CountryIsoCode::MA;
  else if (code == "MZ")
    return CountryIsoCode::MZ;
  else if (code == "MM")
    return CountryIsoCode::MM;
  else if (code == "NA")
    return CountryIsoCode::NA;
  else if (code == "NR")
    return CountryIsoCode::NR;
  else if (code == "NP")
    return CountryIsoCode::NP;
  else if (code == "NL")
    return CountryIsoCode::NL;
  else if (code == "NC")
    return CountryIsoCode::NC;
  else if (code == "NZ")
    return CountryIsoCode::NZ;
  else if (code == "NI")
    return CountryIsoCode::NI;
  else if (code == "NE")
    return CountryIsoCode::NE;
  else if (code == "NG")
    return CountryIsoCode::NG;
  else if (code == "NU")
    return CountryIsoCode::NU;
  else if (code == "NF")
    return CountryIsoCode::NF;
  else if (code == "MP")
    return CountryIsoCode::MP;
  else if (code == "NO")
    return CountryIsoCode::NO;
  else if (code == "OM")
    return CountryIsoCode::OM;
  else if (code == "PK")
    return CountryIsoCode::PK;
  else if (code == "PW")
    return CountryIsoCode::PW;
  else if (code == "PS")
    return CountryIsoCode::PS;
  else if (code == "PA")
    return CountryIsoCode::PA;
  else if (code == "PG")
    return CountryIsoCode::PG;
  else if (code == "PY")
    return CountryIsoCode::PY;
  else if (code == "PE")
    return CountryIsoCode::PE;
  else if (code == "PH")
    return CountryIsoCode::PH;
  else if (code == "PN")
    return CountryIsoCode::PN;
  else if (code == "PL")
    return CountryIsoCode::PL;
  else if (code == "PT")
    return CountryIsoCode::PT;
  else if (code == "PR")
    return CountryIsoCode::PR;
  else if (code == "QA")
    return CountryIsoCode::QA;
  else if (code == "RE")
    return CountryIsoCode::RE;
  else if (code == "RO")
    return CountryIsoCode::RO;
  else if (code == "RU")
    return CountryIsoCode::RU;
  else if (code == "RW")
    return CountryIsoCode::RW;
  else if (code == "BL")
    return CountryIsoCode::BL;
  else if (code == "SH")
    return CountryIsoCode::SH;
  else if (code == "KN")
    return CountryIsoCode::KN;
  else if (code == "LC")
    return CountryIsoCode::LC;
  else if (code == "MF")
    return CountryIsoCode::MF;
  else if (code == "PM")
    return CountryIsoCode::PM;
  else if (code == "VC")
    return CountryIsoCode::VC;
  else if (code == "WS")
    return CountryIsoCode::WS;
  else if (code == "SM")
    return CountryIsoCode::SM;
  else if (code == "ST")
    return CountryIsoCode::ST;
  else if (code == "SA")
    return CountryIsoCode::SA;
  else if (code == "SN")
    return CountryIsoCode::SN;
  else if (code == "RS")
    return CountryIsoCode::RS;
  else if (code == "SC")
    return CountryIsoCode::SC;
  else if (code == "SL")
    return CountryIsoCode::SL;
  else if (code == "SG")
    return CountryIsoCode::SG;
  else if (code == "SX")
    return CountryIsoCode::SX;
  else if (code == "SK")
    return CountryIsoCode::SK;
  else if (code == "SI")
    return CountryIsoCode::SI;
  else if (code == "SB")
    return CountryIsoCode::SB;
  else if (code == "SO")
    return CountryIsoCode::SO;
  else if (code == "ZA")
    return CountryIsoCode::ZA;
  else if (code == "GS")
    return CountryIsoCode::GS;
  else if (code == "SS")
    return CountryIsoCode::SS;
  else if (code == "ES")
    return CountryIsoCode::ES;
  else if (code == "LK")
    return CountryIsoCode::LK;
  else if (code == "SD")
    return CountryIsoCode::SD;
  else if (code == "SR")
    return CountryIsoCode::SR;
  else if (code == "SJ")
    return CountryIsoCode::SJ;
  else if (code == "SZ")
    return CountryIsoCode::SZ;
  else if (code == "SE")
    return CountryIsoCode::SE;
  else if (code == "CH")
    return CountryIsoCode::CH;
  else if (code == "SY")
    return CountryIsoCode::SY;
  else if (code == "TJ")
    return CountryIsoCode::TJ;
  else if (code == "TZ")
    return CountryIsoCode::TZ;
  else if (code == "TH")
    return CountryIsoCode::TH;
  else if (code == "TL")
    return CountryIsoCode::TL;
  else if (code == "TG")
    return CountryIsoCode::TG;
  else if (code == "TK")
    return CountryIsoCode::TK;
  else if (code == "TO")
    return CountryIsoCode::TO;
  else if (code == "TT")
    return CountryIsoCode::TT;
  else if (code == "TN")
    return CountryIsoCode::TN;
  else if (code == "TR")
    return CountryIsoCode::TR;
  else if (code == "TM")
    return CountryIsoCode::TM;
  else if (code == "TC")
    return CountryIsoCode::TC;
  else if (code == "TV")
    return CountryIsoCode::TV;
  else if (code == "UG")
    return CountryIsoCode::UG;
  else if (code == "UA")
    return CountryIsoCode::UA;
  else if (code == "AE")
    return CountryIsoCode::AE;
  else if (code == "UM")
    return CountryIsoCode::UM;
  else if (code == "UY")
    return CountryIsoCode::UY;
  else if (code == "UZ")
    return CountryIsoCode::UZ;
  else if (code == "VU")
    return CountryIsoCode::VU;
  else if (code == "VE")
    return CountryIsoCode::VE;
  else if (code == "VN")
    return CountryIsoCode::VN;
  else if (code == "VG")
    return CountryIsoCode::VG;
  else if (code == "VI")
    return CountryIsoCode::VI;
  else if (code == "WF")
    return CountryIsoCode::WF;
  else if (code == "EH")
    return CountryIsoCode::EH;
  else if (code == "YE")
    return CountryIsoCode::YE;
  else if (code == "ZM")
    return CountryIsoCode::ZM;
  else if (code == "ZW")
    return CountryIsoCode::ZW;
  else
    return CountryIsoCode::XX;
}
}  // namespace utils
}  // namespace nekit
