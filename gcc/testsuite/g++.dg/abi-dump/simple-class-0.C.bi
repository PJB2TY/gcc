<abi-instr version='1.0'>
  <type-decl name='int' size-in-bits='32' alignment-in-bits='32' id='type-id-1'/>
  <class-decl name='S' visibility='default' is-declaration-only='yes' id='type-id-2'/>
  <pointer-type-def type-id='type-id-2' size-in-bits='64' alignment-in-bits='64' id='type-id-3'/>
  <type-decl name='void' alignment-in-bits='8' id='type-id-4'/>
  <qualified-type-def type-id='type-id-2' const='yes' filepath='simple-class-0.C' line='5' column='8' id='type-id-5'/>
  <reference-type-def kind='lvalue' type-id='type-id-5' size-in-bits='64' alignment-in-bits='64' id='type-id-6'/>
  <class-decl name='S' size-in-bits='32' alignment-in-bits='32' visibility='default' filepath='simple-class-0.C' line='5' column='8' def-of-decl-id='type-id-2' id='type-id-7'>
    <member-type access='public'>
      <typedef-decl name='S' type-id='type-id-2' filepath='simple-class-0.C' line='6' column='1' id='type-id-8'/>
    </member-type>
    <data-member access='public' layout-offset-in-bits='0'>
      <var-decl name='m' type-id='type-id-1' visibility='default' filepath='simple-class-0.C' line='7' column='7'/>
    </data-member>
    <member-function access='public' constructor='yes'>
      <function-decl name='__base_ctor ' mangled-name='_ZN1SC2Ev' filepath='simple-class-0.C' line='13' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-3'/>
        <return type-id='type-id-4'/>
      </function-decl>
    </member-function>
    <member-function access='public' constructor='yes'>
      <function-decl name='__comp_ctor ' mangled-name='_ZN1SC1Ev' filepath='simple-class-0.C' line='13' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-3'/>
        <return type-id='type-id-4'/>
      </function-decl>
    </member-function>
    <member-function access='public' constructor='yes'>
      <function-decl name='__base_ctor ' mangled-name='_ZN1SC2ERKS_' filepath='simple-class-0.C' line='18' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-3'/>
        <parameter type-id='type-id-6'/>
        <return type-id='type-id-4'/>
      </function-decl>
    </member-function>
    <member-function access='public' constructor='yes'>
      <function-decl name='__comp_ctor ' mangled-name='_ZN1SC1ERKS_' filepath='simple-class-0.C' line='18' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-3'/>
        <parameter type-id='type-id-6'/>
        <return type-id='type-id-4'/>
      </function-decl>
    </member-function>
  </class-decl>
  <reference-type-def kind='lvalue' type-id='type-id-7' size-in-bits='64' alignment-in-bits='64' id='type-id-9'/>
  <function-decl name='foo' mangled-name='_Z3fooR1S' filepath='simple-class-0.C' line='24' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
    <parameter type-id='type-id-9'/>
    <return type-id='type-id-4'/>
  </function-decl>
</abi-instr>
