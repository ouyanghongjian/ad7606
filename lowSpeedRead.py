from ctypes import *
import types
import time
import select


class RawStruct(Structure):
    _fields_ = [("ua", c_float), ("ub", c_float), ("uc", c_float), ("i0", c_float), ("u0", c_float)]


# 低速读取类
class LowSpeedRead(object):
    def __init__(self):
        self.cdll = cdll.LoadLibrary('./libadcraw.so')
        self.vReferenceValue = 1  # 电压基准值(自行修改)
        self.iReferenceValue = 1  # 电流基准值(自行修改)
        self.transData_Va = [0]*1000   # Va
        self.transData_Vb = [0]*1000   # Vb
        self.transData_Vc = [0]*1000   # Vc
        self.transData_I0 = [0]*1000   # I0
        self.transData_U0 = [0]*1000  # U0

    '''
        输入数据格式如下:struct[list1[Ua, Ub, Uc, I0, U0], list2[Ua, Ub, Uc, I0, U0], ...
                               list999[Ua, Ub, Uc, I0, U0], list1000[Ua, Ub, Uc, I0, U0]]
        单次发1000组
    '''

    def initAdc(self):
        self.cdll.init_m3f20xm()

    def releasAdc(self):
        self.cdll.release_m3f20xm()

    def stopAdc(self):
        self.cdll.stop_adc()

    def returnData(self):
        self.cdll.get_adc_value.restype = POINTER(RawStruct)
        adcarry = self.cdll.get_adc_value()  # 获得低速读取数据
        for i in range(1000):
            self.transData_Va[i] = adcarry[i].ua * self.vReferenceValue  # Va，需结合测试结果自行改
            self.transData_Vb[i] = adcarry[i].ub * self.vReferenceValue  # Vb，需结合测试结果自行改
            self.transData_Vc[i] = adcarry[i].uc * self.vReferenceValue  # Vc，需结合测试结果自行改
            self.transData_I0[i] = adcarry[i].i0 * self.iReferenceValue  # I0，需结合测试结果自行改
            self.transData_U0[i] = adcarry[i].u0 * self.iReferenceValue  # U0, 需结合测试结果自行改识

        print(self.transData_Va)
        return self.transData_Va, self.transData_Vb, self.transData_Vc,self.transData_I0, self.transData_U0


a = LowSpeedRead()

a.initAdc()
time.sleep(0.1)
for j in range(10):
    for i in range(10):
        a.returnData()
        time.sleep(0.02)
    a.stopAdc()
    time.sleep(0.1)
a.releasAdc()
time.sleep(1)
