## AIDistance (AI적 거리두기) Team ReadMe file ##

1. syn
  (경로) evaluation/evaluation/evaluation_AIDistance/hw/design/2020_ai_semicon_design_contest_AIDistance/projects/confmc/confmc.hw/hw/iplib/user_inference_ip/syn/nexys_video 로 이동
  $ make

    구조
        - user_inference_ip/rtl/verilog : user_inference_ip verilog code
        - user_inference_ip/rtl/ip : BRAM ip

    참고사항
        - 현재 verilog code encryption 문제로 Cadence tool로 encrytion 한 코드가 있어서 make 하면 오류 발생
        - project는 열림
    
2. gen_ip
  (경로) evaluation/evaluation/evaluation_AIDistance/hw/design/2020_ai_semicon_design_contest_AIDistance/projects/confmc/confmc.hw/hw/iplib/user_inference_ip/gen_ip/nexys_video 로 이동
  $ make

3. impl
  (경로) evaluation/evaluation/evaluation_AIDistance/hw/design/2020_ai_semicon_design_contest_AIDistance/projects/confmc/confmc.hw/hw/impl/vivado.nexys_video.confmc.pure 로 이동
  $ make open    : open vivado GUI

  