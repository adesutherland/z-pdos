* SPDX-License-Identifier: MIT; original selected-layout consumer.
         CVT DSECT=YES
         IKJTCB
         IEZJSCB
         IHAPSA
         IHARB
         IHACDE
         IHASVC
         IHAASCB
         IHAASXB
MAPTEST  CSECT
         DC H'FLCSOPSW-PSA'
         DC H'FLCCSW-PSA'
         DC H'FLCSNPSW-PSA'
         DC H'FLCPNPSW-PSA'
         DC H'FLCMNPSW-PSA'
         DC H'FLCINPSW-PSA'
         DC H'FLCFLA-PSA'
         DC H'FLCGRSAV-PSA'
         DC H'PSATOLD-PSA'
         DC H'PSAANEW-PSA'
         DC H'PSAAOLD-PSA'
         DC H'PSACLHT-PSA'
         DC H'SVCOPSW-PSA'
         DC AL2(L'FLCFLA)
         DC AL2(L'FLCGRSAV)
         DC H'TCBRBP-TCB'
         DC H'TCBDEB-TCB'
         DC H'TCBTIO-TCB'
         DC H'TCBCMP-TCB'
         DC H'JSCBPSCB-IEZJSCB'
         DC H'RBCDE1-RBBASIC'
         DC H'CDNAME-CDENTRY'
         DC AL2(L'CDNAME)
         DC H'CDENTPT-CDENTRY'
         DC H'ASCBASXB-ASCB'
         DC H'ASXBLWA-ASXB'
         DC H'CVTTZ-CVTMAP'
         DC H'CVTPTR'
         DC H'SVCEP-SVCENTRY'
         DC H'SVCTP-SVCENTRY'
         DC H'SVCATTR3-SVCENTRY'
         DC H'SVCLOCKS-SVCENTRY'
         END MAPTEST
