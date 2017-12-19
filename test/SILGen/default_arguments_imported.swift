// RUN: %target-swift-frontend -sdk %S/Inputs -I %S/Inputs -enable-source-import %s -emit-silgen -enable-sil-ownership | %FileCheck %s

// Test SIL generation for imported default arguments.

// REQUIRES: objc_interop

import gizmo

// CHECK-LABEL: sil hidden @_T026default_arguments_imported9testGizmo{{[_0-9a-zA-Z]*}}F
func testGizmo(gizmo: Gizmo) {
  // CHECK: function_ref @_T0Sq10nilLiteralxSgyt_tcfC
  // CHECK: objc_method [[SELF:%[0-9]+]] : $Gizmo, #Gizmo.enumerateSubGizmos!1.foreign
  gizmo.enumerateSubGizmos()
} // CHECK: } // end sil function '_T026default_arguments_imported9testGizmo5gizmoySo0E0C_tF'

// CHECK-LABEL: sil hidden @_T026default_arguments_imported21testNonnullDictionary5gizmoySo5GizmoC_tF
func testNonnullDictionary(gizmo: Gizmo) {
  // CHECK-NOT: nilLiteral
  // CHECK: function_ref @_T0s10DictionaryV17dictionaryLiteralAByxq_Gx_q_td_tcfC
  // CHECK: objc_method [[SELF:%[0-9]+]] : $Gizmo, #Gizmo.doTheThing!1.foreign
  gizmo.doTheThing()
} // CHECK: } // end sil function '_T026default_arguments_imported21testNonnullDictionary5gizmoySo5GizmoC_tF'

// CHECK-LABEL: sil hidden @_T026default_arguments_imported22testNullableDictionary5gizmoySo5GizmoC_tF
func testNullableDictionary(gizmo: Gizmo) {
  // CHECK-NOT: dictionaryLiteral
  // CHECK: function_ref @_T0Sq10nilLiteralxSgyt_tcfC
  // CHECK: objc_method [[SELF:%[0-9]+]] : $Gizmo, #Gizmo.doTheOtherThing!1.foreign
  gizmo.doTheOtherThing()
} // CHECK: } // end sil function '_T026default_arguments_imported22testNullableDictionary5gizmoySo5GizmoC_tF'
